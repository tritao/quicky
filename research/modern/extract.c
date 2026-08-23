/*
 * Safe, portable variant of Simon Laburda's extract.c.
 *
 * NESTLE.DAT stores its directory in little-endian order.  The original
 * utility reads past the directory into the final trailer; on modern libc
 * that can turn the trailer into a negative length and abort.  This version
 * stops at the directory boundary and uses bounded, chunked I/O.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t get_u16le(const unsigned char *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t get_u32le(const unsigned char *p) {
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static int copyout(FILE *src, const char *target, uint32_t pos, uint32_t size) {
    unsigned char buf[64 * 1024];
    FILE *dest;
    long remember = ftell(src);

    printf("Copying %u bytes from %u to %s\n", size, pos, target);
    dest = fopen(target, "wb");
    if (!dest) {
        perror(target);
        return 0;
    }
    if (fseek(src, (long)pos, SEEK_SET) != 0) {
        perror("seeking archive payload");
        fclose(dest);
        return 0;
    }
    while (size != 0) {
        size_t want = size < sizeof(buf) ? size : sizeof(buf);
        size_t got = fread(buf, 1, want, src);
        if (got != want || fwrite(buf, 1, got, dest) != got) {
            perror("copying archive payload");
            fclose(dest);
            return 0;
        }
        size -= (uint32_t)got;
    }
    fclose(dest);
    if (fseek(src, remember, SEEK_SET) != 0) {
        perror("restoring directory position");
        return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    const char *archive = argc > 1 ? argv[1] : "NESTLE.DAT";
    FILE *fp = fopen(archive, "rb");
    long file_size;
    unsigned char trailer[8];
    uint32_t dirpos;
    uint32_t directory_end;
    uint32_t previous_pos = 0;
    char previous_name[256] = {0};
    int have_previous = 0;

    if (!fp) {
        perror(archive);
        return 1;
    }
    if (fseek(fp, 0, SEEK_END) != 0 || (file_size = ftell(fp)) < 8) {
        fprintf(stderr, "Invalid or truncated archive\n");
        fclose(fp);
        return 1;
    }
    if (fseek(fp, file_size - 8, SEEK_SET) != 0 ||
        fread(trailer, 1, sizeof(trailer), fp) != sizeof(trailer)) {
        fprintf(stderr, "Cannot read archive trailer\n");
        fclose(fp);
        return 1;
    }

    dirpos = get_u32le(trailer);
    directory_end = (uint32_t)(file_size - 8);
    if (dirpos > directory_end) {
        fprintf(stderr, "Directory offset is outside archive\n");
        fclose(fp);
        return 1;
    }
    printf("Main directory @ %x\n", dirpos);

    if (fseek(fp, (long)dirpos, SEEK_SET) != 0) {
        perror("seeking directory");
        fclose(fp);
        return 1;
    }
    while ((uint32_t)ftell(fp) < directory_end) {
        unsigned char lenbuf[2];
        unsigned char posbuf[4];
        uint16_t name_len;
        uint32_t pos;
        char name[256];

        if (fread(lenbuf, 1, sizeof(lenbuf), fp) != sizeof(lenbuf))
            break;
        name_len = get_u16le(lenbuf);
        if (name_len == 0 || name_len >= sizeof(name)) {
            fprintf(stderr, "Invalid filename length %u\n", name_len);
            fclose(fp);
            return 1;
        }
        if (fread(name, 1, name_len, fp) != name_len ||
            fread(posbuf, 1, sizeof(posbuf), fp) != sizeof(posbuf)) {
            fprintf(stderr, "Truncated directory entry\n");
            fclose(fp);
            return 1;
        }
        name[name_len] = '\0';
        pos = get_u32le(posbuf);
        if (pos > dirpos || (have_previous && pos < previous_pos)) {
            fprintf(stderr, "Invalid payload offset for %s\n", name);
            fclose(fp);
            return 1;
        }
        if (have_previous && !copyout(fp, previous_name, previous_pos, pos - previous_pos)) {
            fclose(fp);
            return 1;
        }
        strcpy(previous_name, name);
        previous_pos = pos;
        have_previous = 1;
    }
    if (have_previous && !copyout(fp, previous_name, previous_pos, dirpos - previous_pos)) {
        fclose(fp);
        return 1;
    }
    fclose(fp);
    return 0;
}
