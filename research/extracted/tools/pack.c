
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct finfo {
	char *fnam;
	int pos;
	struct finfo *next;
};

char buf[1024];
main(int argc, char **argv) {
	struct finfo *anchor = NULL;
	if(!argv[1]) {
		printf("Usage: %s <target> <file to pack> ...\n",argv[1]);
		exit(1);
	}
	if(!argv[2]) {
		puts("No files to pack!");
	}

	unlink(argv[1]);
	FILE *dest = fopen(argv[1],"wb");
	int pos;
	int fidx=2;
	struct finfo *last = NULL;
	while(argv[fidx]) {
		struct finfo *current = malloc(sizeof(struct finfo));
		current->pos = ftell(dest);
		current->fnam = argv[fidx];
		current->next = NULL;
		printf("packing '%s' to '%s@%x'\n", argv[fidx], argv[1], current->pos);
		FILE *src = fopen(argv[fidx],"rb");
		int i;
		while((i = fread(buf, 1, 1024, src)) > 0)
			fwrite(buf, 1, i, dest);
		fclose(src);

		if(last == NULL) {
			anchor = current;
		} else {
			last->next = current;
		}
		last = current;
		fidx++;
	}
	int dirpos = ftell(dest);
	printf("Directory will be at %x\n", dirpos);
	last = anchor;
	while(last) {
		short fnlen = strlen(last->fnam);
		fwrite(&fnlen, 1, 2, dest);
		fwrite(last->fnam, 1, fnlen, dest);
		fwrite(&last->pos, 1, 4, dest);
		last = last->next;
	}
	fwrite(&dirpos, 1, 4, dest);
	argc -= 3;
	fwrite(&argc, 1, 4, dest);
}
