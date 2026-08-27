// Dump little-endian data words from a focused Ghidra program.
// Usage: -postScript DumpDataWords.java <output-file> <offset> <word-count> ...

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;

public class DumpDataWords extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 3 || ((args.length - 1) % 2) != 0) {
            printerr("Usage: DumpDataWords.java <output-file> <offset> <word-count> ...");
            return;
        }
        File output = new File(args[0]);
        File parent = output.getParentFile();
        if (parent != null && !parent.exists() && !parent.mkdirs())
            throw new Exception("Could not create output directory: " + parent);
        try (PrintWriter writer = new PrintWriter(output, "UTF-8")) {
            for (int i = 1; i < args.length; i += 2) {
                int offset = Integer.parseInt(args[i], 16);
                int count = Integer.parseInt(args[i + 1]);
                writer.printf("; ---- %04X words=%d ----%n", offset, count);
                for (int n = 0; n < count; n++) {
                    int addressOffset = offset + n * 2;
                    Address address = toAddr(addressOffset);
                    int lo = currentProgram.getMemory().getByte(address) & 0xff;
                    int hi = currentProgram.getMemory().getByte(toAddr(addressOffset + 1)) & 0xff;
                    writer.printf("%04X: %04X%n", addressOffset, lo | (hi << 8));
                }
                writer.println();
            }
        }
        println("Wrote " + output.getAbsolutePath());
    }
}
