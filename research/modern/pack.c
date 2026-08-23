/*
 * Safe, portable variant of Simon Laburda's pack.c.
 * Usage: pack <target> <file to pack> ...
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct finfo {
    const char *name;
    uint32_t pos;
    struct finfo *next;
};

static void put_u16le(FILE *fp, uint16_t value) {
    fputc(value & 0xff, fp);
    fputc((value >> 8) & 0xff, fp);
}

static void put_u32le(FILE *fp, uint32_t value) {
    fputc(value & 0xff, fp);
    fputc((value >> 8) & 0xff, fp);
    fputc((value >> 16) & 0xff, fp);
    fputc((value >> 24) & 0xff, fp);
}

int main(int argc, char **argv) {
    unsigned char buf[64 * 1024];
    struct finfo *head = NULL;
    struct finfo *tail = NULL;
    FILE *dest;
    uint32_t dirpos;
    int i;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <target> <file to pack> ...\n", argv[0]);
        return 1;
    }
    dest = fopen(argv[1], "wb");
    if (!dest) {
        perror(argv[1]);
        return 1;
    }
    for (i = 2; i < argc; ++i) {
        FILE *src = fopen(argv[i], "rb");
        struct finfo *entry;
        size_t got;
        size_t name_len = strlen(argv[i]);

        if (!src) {
            perror(argv[i]);
            fclose(dest);
            return 1;
        }
        if (name_len == 0 || name_len > 0xffff) {
            fprintf(stderr, "Filename is too long: %s\n", argv[i]);
            fclose(src);
            fclose(dest);
            return 1;
        }
        entry = (struct finfo *)malloc(sizeof(*entry));
        if (!entry) {
            perror("malloc");
            fclose(src);
            fclose(dest);
            return 1;
        }
        entry->name = argv[i];
        entry->pos = (uint32_t)ftell(dest);
        entry->next = NULL;
        if (tail)
            tail->next = entry;
        else
            head = entry;
        tail = entry;

        printf("packing '%s' to '%s@%x'\n", argv[i], argv[1], entry->pos);
        while ((got = fread(buf, 1, sizeof(buf), src)) != 0) {
            if (fwrite(buf, 1, got, dest) != got) {
                perror("writing archive");
                fclose(src);
                fclose(dest);
                return 1;
            }
        }
        fclose(src);
    }

    dirpos = (uint32_t)ftell(dest);
    printf("Directory will be at %x\n", dirpos);
    for (struct finfo *entry = head; entry; entry = entry->next) {
        size_t name_len = strlen(entry->name);
        put_u16le(dest, (uint16_t)name_len);
        fwrite(entry->name, 1, name_len, dest);
        put_u32le(dest, entry->pos);
    }
    put_u32le(dest, dirpos);
    /* The original format stores number_of_files - 1. */
    put_u32le(dest, (uint32_t)(argc - 3));
    fclose(dest);

    while (head) {
        struct finfo *next = head->next;
        free(head);
        head = next;
    }
    return 0;
}
