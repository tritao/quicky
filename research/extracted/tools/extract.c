#include <stdio.h>
#include <stdlib.h>

void copyout(FILE *src, char *targ, int pos, int size) {
	printf("Copying %i bytes from %i to %s\n", size, pos, targ);
	FILE *dest = fopen(targ, "wb");
	if(!dest) {
		perror(targ);
		exit(2);
	}
	int remember = ftell(src);

	char *buf = malloc(size);
	fseek(src, pos, SEEK_SET);
	fread(buf, size, 1, src);
	fwrite(buf, size, 1, dest);
	fclose(dest);
	free(buf);

	fseek(src, remember, SEEK_SET);
}

main(int argc, char **argv) {
	FILE *fp = fopen(argv[1] ? argv[1] : "NESTLE.DAT","rb");
	if(fp == NULL) {
		perror("fopen nestle.dat");
		exit(1);
	}

	int dirpos;
	fseek(fp, -8, SEEK_END);
	fread(&dirpos, 4, 1, fp);
	printf("Main directory @ %x\n", dirpos);

	short len;
	char fnam[16], oldfnam[16]={0};
	int pos, oldpos;
	fseek(fp, dirpos, SEEK_SET);
	for(;;) {
		if(fread(&len, 2, 1, fp) < 1) break;
		if(len >= 16) break;
		if(fread(fnam, len, 1, fp) < 1) break;
		fnam[len] = 0;
		if(fread(&pos, 4, 1, fp) < 1) break;

		if(oldfnam[0])
			copyout(fp, oldfnam, oldpos, pos - oldpos);
//		printf("File '%s' @ %x\n",fnam, pos);
		strcpy(oldfnam, fnam);
		oldpos = pos;
	}
	if(oldfnam[0])
		copyout(fp, oldfnam, oldpos, dirpos - pos);



}
