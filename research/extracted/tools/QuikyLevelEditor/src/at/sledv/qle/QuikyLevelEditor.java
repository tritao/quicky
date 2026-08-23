/*
 * QuikyLevelEditor.java
 *
 * Created by Simon Laburda on 28.11.2010, 16:32:45
 */

package at.sledv.qle;

import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Rectangle;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.JViewport;
import javax.swing.filechooser.FileFilter;

/**
 *
 * @author simbone
 */
public class QuikyLevelEditor extends javax.swing.JFrame  {

    int map[][];
    int garbage;
    ArrayList<BufferedImage> tiles = new ArrayList();
    Color pal[] = new Color[256];
    File current;

    class MapThingy extends Component implements MouseListener, MouseMotionListener {
        MapThingy() {
            addMouseListener(this);
            addMouseMotionListener(this);
        }
        final void drawTile(Graphics g, int x, int y) {
            if(x >= 0 && x < map.length && y >= 0 && y < map[0].length) {
                int tile = map[x][y];
                tile &= 0x1FF;
                try {
                    g.drawImage(tiles.get(tile), x*16, y*16, null);
                } catch(IndexOutOfBoundsException ex) { }
            }
        }
        public void paint(Graphics g) {
            Rectangle r = g.getClipBounds();
            int min_x = (int) r.getX() / 16;
            int min_y = (int) r.getY() / 16;
            int max_x = ((int) (r.getX()+r.getWidth())+15) / 16;
            int max_y = ((int) (r.getY()+r.getHeight())+15) / 16;
            //System.out.println("Drawing "+min_x+"x"+min_y + " to "+max_x+"x"+max_y);
            for(int y=min_y;y<=max_y;y++) {
                for(int x=min_x;x<=max_x;x++) {
                    drawTile(g,x,y);
                }
            }
            if(lastx != -1 || lasty != -1) {
                g.setColor(Color.RED);
                g.drawRect(lastx*16, lasty*16,16,16);
            }
        }
        public void update() {
            Dimension dim = new Dimension(map.length*16, map[0].length*16);
            setMinimumSize(dim);
            setMaximumSize(dim);
            setPreferredSize(dim);
            setSize(dim);
            getParent().getParent().validate();
            repaint();
        }

        public void mouseClicked(MouseEvent e) {
        }

        public void mousePressed(MouseEvent e) {
        }

        public void mouseReleased(MouseEvent e) {
        }

        public void mouseEntered(MouseEvent e) {
        }

        public void mouseExited(MouseEvent e) {
        }

        public void mouseDragged(MouseEvent e) {
        }

        int lastx,lasty;
        public void mouseMoved(MouseEvent e) {
            int x = e.getX() / 16;
            int y = e.getY() / 16;
            if(x < 0 || y < 0 || x >= map.length || y >= map[0].length) return;
            if(x != lastx || y != lasty) {
                Graphics g = getGraphics();
                drawTile(g, lastx, lasty);
                lastx = x;
                lasty = y;
                g.setColor(Color.RED);
                g.drawRect(x*16, y*16, 15,15);

                int data = map[x][y];
                lblStatus.setText(Integer.toHexString(data));
            }
        }
    }
    MapThingy mapThingy;

    /** Creates new form QuikyLevelEditor */
    public QuikyLevelEditor() {
        initComponents();
        mapThingy = new MapThingy();
        JViewport jv = new JViewport();
        jv.setView(mapThingy);
        jScrollPane1.setViewport(jv);
    }

    public void openMap(String path) {
        try {
            current = new File(path);
            int i;
            InputStream is = new FileInputStream(path);
            if(is.read() != 'T') throw new IOException("Not a TLE1 Mapfile");
            if(is.read() != 'L') throw new IOException("Not a TLE1 Mapfile");
            if(is.read() != 'E') throw new IOException("Not a TLE1 Mapfile");
            if(is.read() != '1') throw new IOException("Not a TLE1 Mapfile");
            int w = is.read();
            w = (w << 8) | is.read();
            int h = is.read();
            h = (h << 8) | is.read();
            garbage = is.read();
            garbage = (garbage << 8) | is.read();

            map = new int[w][h];
            for(int y=0;y<h;y++) {
                for(int x=0;x<w;x++) {
                    int d = is.read();
                    map[x][y] = (d << 8) | is.read();
                }
            }
            is.close();

            is = new FileInputStream(path.substring(0, path.length()-6) + ".PCC");
            if(is.read() != 0x0A) throw new IOException("Not a PCX File");
            is.read(); is.read();
            if(is.read() != 0x08) throw new IOException("Not a 256 color PCX File");
            is.skip(0x80 - 4);
            while((i=is.read()) != 0x0C)
                if(i < 0)
                    throw new IOException("PCX palette missing");

            for(i=0;i<256;i++) {
                int r = is.read();
                int g = is.read();
                int b = is.read();
                pal[i] = new Color(r,g,b);
            }
            is.close();

            is = new FileInputStream(path.substring(0, path.length()-6) + ".ICO");

            tiles.clear();
            byte buf[] = new byte[256];
            boolean kellmap = false;
            out: for(;;) {
                int pos=0;
                do {
                    int num = is.read(buf,pos,256-pos);
                    if(num <= 0) break out;
                    pos += num;
                } while(pos < 256);

                if(tiles.size() == 0 && (((int) buf[0]) & 0xFF) >= 0x80) {
                    kellmap = true; // this is a kelloggs style variant. the palette is a little different
                    System.out.println("Kelloggs style map detected");
                }
                BufferedImage bi = new BufferedImage(16,16,BufferedImage.TYPE_INT_RGB);
                int idx=0;
                for(int y=0;y<16;y++) {
                    for(int x=0;x<16;x++,idx++) {
                        int tent = (((int) buf[idx]) & 0xFF);
                        if(kellmap) {
                            if(tent >= 0xA0)
                                tent = (tent - 0xA0)+32;
                            else if(tent >= 0x90)
                                tent = (tent - 0x90)+16;
                        }
                        bi.setRGB(
                                ((x * 4) & 0x0F) + (x >> 2),
                                y,
                                pal[tent].getRGB()
                            );
                    }
                }
                tiles.add(bi);
            }
            mapThingy.update();
            System.out.println("Open done. Map is "+map.length+"x"+map[0].length+", we have " + tiles.size() + " tiles");
        } catch(Exception ex) {
            JOptionPane.showMessageDialog(this, ex.getClass().getName() + ": " + ex.getMessage());
            ex.printStackTrace();
        }
    }

    /** This method is called from within the constructor to
     * initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is
     * always regenerated by the Form Editor.
     */
    @SuppressWarnings("unchecked")
    // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
    private void initComponents() {

        jScrollPane1 = new javax.swing.JScrollPane();
        lblStatus = new javax.swing.JLabel();
        jMenuBar1 = new javax.swing.JMenuBar();
        mnuFile = new javax.swing.JMenu();
        mnuOpen = new javax.swing.JMenuItem();

        setDefaultCloseOperation(javax.swing.WindowConstants.EXIT_ON_CLOSE);
        setTitle("Quiky & Kelloggs Level editor");
        getContentPane().add(jScrollPane1, java.awt.BorderLayout.CENTER);

        lblStatus.setText("jLabel1");
        getContentPane().add(lblStatus, java.awt.BorderLayout.SOUTH);

        mnuFile.setText("File");

        mnuOpen.setText("Open");
        mnuOpen.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                mnuOpenActionPerformed(evt);
            }
        });
        mnuFile.add(mnuOpen);

        jMenuBar1.add(mnuFile);

        setJMenuBar(jMenuBar1);

        java.awt.Dimension screenSize = java.awt.Toolkit.getDefaultToolkit().getScreenSize();
        setBounds((screenSize.width-492)/2, (screenSize.height-388)/2, 492, 388);
    }// </editor-fold>//GEN-END:initComponents

    private void mnuOpenActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_mnuOpenActionPerformed
        JFileChooser jfc = new JFileChooser();
        jfc.setSelectedFile(current);
        jfc.setDialogTitle("Open MAP file");
        jfc.setFileFilter(new FileFilter() {
            @Override
            public boolean accept(File f) {
                return f.isDirectory() || f.toString().toLowerCase().endsWith(".map");
            }

            @Override
            public String getDescription() {
                return "QUIKY Map Files";
            }
        });
        if(jfc.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
            openMap(jfc.getSelectedFile().toString());
        }
    }//GEN-LAST:event_mnuOpenActionPerformed

    /**
    * @param args the command line arguments
    */
    public static void main(final String args[]) {

        java.awt.EventQueue.invokeLater(new Runnable() {
            public void run() {
                QuikyLevelEditor qle = new QuikyLevelEditor();
                qle.setVisible(true);
                if(args.length > 0)
                    qle.openMap(args[0]);
            }
        });
    }

    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JMenuBar jMenuBar1;
    private javax.swing.JScrollPane jScrollPane1;
    private javax.swing.JLabel lblStatus;
    private javax.swing.JMenu mnuFile;
    private javax.swing.JMenuItem mnuOpen;
    // End of variables declaration//GEN-END:variables

}
