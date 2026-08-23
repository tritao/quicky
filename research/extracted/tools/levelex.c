
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#pragma pack(1)
struct MapHeader {
	char magic[4];		// "TLE1"
	uint16_t width;		// BIG-ENDIAN!
	uint16_t height;	// BIG-ENDIAN!
	uint16_t unknown;	// no idea...
};

inline int v(uint16_t val) {
	return ((val >> 8) & 0x00FF) | ((val << 8) & 0xFF00);
}

char charusage[0x10000];
int charpos=0;
char *chars = " .ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

main(int argc, char **argv) {

	if(argv[1] == NULL) {
		printf("Usage: %s <mapfile>\n", argv[0]);
	}
	FILE *fp;

	if((fp = fopen(argv[1], "rb")) == NULL) {
		perror("Opening map");
		exit(1);
	}
	struct MapHeader hdr;

	if(fread(&hdr, sizeof(hdr), 1, fp) < 1) {
		perror("Short read on map file");
		exit(1);
	}
	if(hdr.magic[0] != 'T' || hdr.magic[1] != 'L' || hdr.magic[2] != 'E' || hdr.magic[3] != '1') {
		perror("Not a TLE1 map file");
		exit(1);
	}
	hdr.width = v(hdr.width);
	hdr.height = v(hdr.height);

	int i,j;
	for(i=0;i<hdr.height;i++) {
		for(j=0;j<hdr.width;j++) {
			int val = fgetc(fp);
			val = (val << 8) | fgetc(fp);

			char c = charusage[val];
			if(c == 0) {
				if(chars[charpos] == 0)
					charpos = 2;
				c = charusage[val] = chars[charpos++];
			}
			putchar(c);
		}
		putchar('\n');
	}
	fclose(fp);
}
