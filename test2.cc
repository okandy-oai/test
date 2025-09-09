#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

static size_t min_size(size_t a, size_t b) { return a < b ? a : b; }

static size_t xcopy(char *dst, size_t dstsz, const char *src) {
    size_t n = dstsz ? dstsz - 1 : 0;
    size_t srclen = strlen(src);
    size_t to_copy = min_size(n, srclen);

    if (to_copy && dst) {
        memcpy(dst, src, to_copy);
    }
    if (dstsz && dst) {
        dst[to_copy] = '\0';
    }
    return srclen;
}

static size_t xappend(char *dst, size_t dstsz, const char *src) {
    size_t used = dst ? strlen(dst) : 0;
    if (used >= dstsz) {
        if (dstsz && dst) dst[dstsz - 1] = '\0';
        return used + strlen(src);
    }
    size_t room = dstsz - used - 1; // space left for content (keeping 1 for NUL)
    size_t srclen = strlen(src);
    size_t to_copy = min_size(room, srclen);

    if (to_copy && dst) {
        memcpy(dst + used, src, to_copy);
    }
    if (dstsz && dst) {
        dst[used + to_copy] = '\0';
    }
    return used + srclen;
}

static void dump(const char *label, const char *buf, size_t bufsz) {
    printf("%s (bufsz=%zu): \"%s\"\n", label, bufsz, buf);
    // Show raw bytes to prove NUL termination and no spill
    printf("  bytes: ");
    for (size_t i = 0; i < bufsz; ++i) {
        unsigned char c = (unsigned char)buf[i];
        if (c >= 32 && c <= 126) printf("%c", c);
        else if (c == 0) printf("\\0");
        else printf(".");
    }
    printf("\n");
}

int main(void) {
    enum { BUF_SZ = 16 };
    char buf[BUF_SZ];

    {
        const char *fifteen = "ABCDEFGHIJKLMNO"; // 15 chars
        size_t needed = xcopy(buf, sizeof(buf), fifteen);
        dump("Exact-fit copy", buf, sizeof(buf));
        printf("  src_len=%zu  truncated? %s\n\n", needed, needed >= sizeof(buf));
    }

    {
        const char *sixteen = "ABCDEFGHIJKLMNOP"; // 16 chars
        size_t needed = xcopy(buf, sizeof(buf), sixteen);
        dump("Off-by-one xcopy", buf, sizeof(buf));
        printf("  src_len=%zu  truncated? %s\n\n", needed, needed >= sizeof(buf));
    }

    {
        char *big = (char *)malloc(128);
        if (!big) return 1;
        memset(big, 'X', 127);
        big[127] = '\0';

        size_t needed = xcopy(buf, sizeof(buf), big);
        dump("Heavy truncation xcopy", buf, sizeof(buf));
        printf("  src_len=%zu  truncated? %s\n\n", needed, needed >= sizeof(buf));

        free(big);
    }

    {
        xcopy(buf, sizeof(buf), "1234567890"); // 10 chars
        size_t intended1 = xappend(buf, sizeof(buf), "ABCDE"); // 5 -> still safe
        dump("After append ABCDE", buf, sizeof(buf));
        printf("  intended_total_len=%zu  truncated? %s\n\n", intended1,
               intended1 >= sizeof(buf));

        // Another append pushes right to the boundary, but no overflow
        size_t intended2 = xappend(buf, sizeof(buf), "QRSTUV"); // 6 -> some truncation
        dump("After append QRSTUV", buf, sizeof(buf));
        printf("  intended_total_len=%zu  truncated? %s\n\n", intended2,
               intended2 >= sizeof(buf));
    }

    {
        // Fill buffer with a known pattern first.
        memset(buf, '#', sizeof(buf));
        buf[sizeof(buf) - 1] = '\0'; // keep it valid C-string
        int n = snprintf(buf, sizeof(buf), "id=%d tag=%s", 42, "near-limit");
        dump("snprintf demo", buf, sizeof(buf));
        printf("  snprintf_ret=%d  truncated? %s\n\n", n, n >= (int)sizeof(buf));
    }

    puts("Completed without overflow. If you compiled with -fsanitize=address,");
    puts("any out-of-bounds would have been reported. ✔️");
    return 0;
}
