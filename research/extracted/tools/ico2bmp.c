#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#pragma pack(1)

typedef struct tagBITMAPFILEHEADER {
  int16_t bfType;
  int32_t bfSize;
  int16_t bfReserved1;
  int16_t bfReserved2;
  int32_t bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
  int32_t biSize;
  int32_t  biWidth;
  int32_t  biHeight;
  int16_t  biPlanes;
  int16_t  biBitCount;
  int32_t biCompression;
  int32_t biSizeImage;
  int32_t  biXPelsPerMeter;
  int32_t  biYPelsPerMeter;
  int32_t biClrUsed;
  int32_t biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

uint8_t *tiledata;
uint8_t *bmpdata;
int bmpwidth, bmpheight;

inline static int getpixel(int tile, int x, int y) {
	int xx = ((x * 4) & 0x0F) + (x >> 2);
	return tiledata[tile*256 + y * 16 + xx];
}

inline static void setpixel(int x, int y, int col) {
	bmpdata[(bmpheight-y-1)*bmpwidth + x] = col;
}

main(int argc, char **argv) {
	uint8_t pcxpal[256*3];
	int i;
	if(argc != 4) {
		printf("Usage: %s <Palette.PCC> <Tileset.ICO> <Output.BMP>\nExample: %s W1.PCC W1.ICO ~/Desktop/world1.bmp\n",argv[0],argv[0]);
		exit(1);
	}
	FILE *fp;
	if((fp = fopen(argv[1],"rb")) == NULL) {
		perror("Error opening palette");
		exit(1);
	}
	if(fgetc(fp) != 0x0a) {	// header
		printf("Palette file not a valid PCX\n");
		exit(1);
	}
	fgetc(fp); fgetc(fp); // ignore version&compression
	if(fgetc(fp) != 0x08) {
		printf("Palette not a 256 color PCX\n");
		exit(1);
	}
	fseek(fp, 0x80, SEEK_SET);	// seek to pixeldata
	while((i = fgetc(fp)) != 0x0C) {	// look for 0C (start of palette)
		if(i < 0) {
			printf("PCX-File doesn't contain a palette\n");
			exit(1);
		}
	}
	if(fread(pcxpal,1, sizeof(pcxpal),fp) != sizeof(pcxpal)) {	// read it in
		perror("Short read on PCX pal.");
		exit(1);
	}
	fclose(fp);	// we're done with the PCX

	if((fp = fopen(argv[2],"rb")) == NULL) {
		perror("Error opening tilefile");
		exit(1);
	}
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	if(size % 256) {
		printf("This is not a valid tilefile. Size not multiple of 256\n");
		exit(2);
	}
	int numtiles = size / 256;
//	int htiles = (int) sqrt(numtiles);
//	int wtiles = numtiles / htiles;

	int wtiles = numtiles > 20 ? 20 : numtiles;
	int htiles = (numtiles + wtiles - 1) / wtiles;
	bmpwidth = wtiles * 16;
	bmpheight = htiles * 16;
	printf("Found %i tiles. Will organize them into a %ix%i tile bitmap\n",numtiles,wtiles,htiles);

	if((tiledata = malloc(size)) == NULL) {
		perror("Out of memory for tile storage");
		exit(2);
	}
	fseek(fp, 0, SEEK_SET);
	if(fread(tiledata,1,size,fp) < size) {
		perror("Short read while getting tile data");
		exit(2);
	}
	fclose(fp);

	unlink(argv[3]);
	if((fp = fopen(argv[3],"wb")) == NULL) {
		perror("Error opening target file");
		exit(3);
	}
	BITMAPFILEHEADER bmp;
	BITMAPINFOHEADER bmpinfo;

	bmp.bfType = 19778;	// "BM"
	bmp.bfSize = sizeof(bmp) +
		sizeof(bmpinfo) +
		256 * 4 +
		htiles * wtiles * 256;
	bmp.bfOffBits = sizeof(bmp) + sizeof(bmpinfo) + 256 * 4;

	bmpinfo.biSize = sizeof(bmpinfo);
	bmpinfo.biWidth = bmpwidth;
	bmpinfo.biHeight = bmpheight;
	bmpinfo.biPlanes = 1;
	bmpinfo.biBitCount = 8;
	bmpinfo.biCompression = 0;
	bmpinfo.biSizeImage = 0;
	bmpinfo.biXPelsPerMeter = 0;
	bmpinfo.biYPelsPerMeter = 0;
	bmpinfo.biClrUsed = 0;	// (= 256)
	bmpinfo.biClrImportant = 0; // (= 256)

	fwrite(&bmp, 1, sizeof(bmp), fp);
	fwrite(&bmpinfo, 1, sizeof(bmpinfo), fp);
	for(i=0;i<256;i++) {
		fputc(pcxpal[i*3+2], fp);
		fputc(pcxpal[i*3+1], fp);
		fputc(pcxpal[i*3+0], fp);
		fputc(0, fp);
	}

	bmpdata = malloc(bmpwidth * bmpheight);

	// transfering the pixeldata
	int x,y;
	for(i=0;i<numtiles;i++) for(y=0;y<16;y++) for(x=0;x<16;x++) {
		int tty = i / wtiles;
		int ttx = i % wtiles;

		setpixel(
			(ttx*16)+x,
			(tty*16)+y,
			getpixel(i, x, y)
		);
	}

	fwrite(bmpdata, 1, bmpwidth * bmpheight, fp);
	fclose(fp);
}
