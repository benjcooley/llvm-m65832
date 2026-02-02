/* test_picolibc_qsort.c - Adapted picolibc qsort tests for M65832
 * Based on picolibc libc-testsuite/qsort.c
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static int err = 0;

static int scmp(const void *a, const void *b) {
    return strcmp(*(char **)a, *(char **)b);
}

static int icmp(const void *a, const void *b) {
    long av = *(long *)a;
    long bv = *(long *)b;
    if (av > bv) return 1;
    if (av < bv) return -1;
    return 0;
}

struct three {
    unsigned char b[3];
};

#define i3(x) { (unsigned char)(((int32_t)(x)) >> 16), (unsigned char)((x) >> 8), (unsigned char)((x) >> 0) }

static int tcmp(const void *av, const void *bv) {
    const struct three *a = av, *b = bv;
    for (int i = 0; i < 3; i++) {
        int c = (int)a->b[i] - (int)b->b[i];
        if (c) return c;
    }
    return 0;
}

/* 26 items -- even */
static char *s[] = { "Bob",    "Alice", "John",   "Ceres",   "Helga",   "Drepper", "Emeralda",
                     "Zoran",  "Momo",  "Frank",  "Pema",    "Xavier",  "Yeva",    "Gedun",
                     "Irina",  "Nono",  "Wiener", "Vincent", "Tsering", "Karnica", "Lulu",
                     "Quincy", "Osama", "Riley",  "Ursula",  "Sam" };

/* 23 items -- odd, prime */
static long n[] = { 879045, 394, 99405644, 33434, 232323, 4334, 5454, 343, 45545, 454, 324, 22,
                    34344, 233, 45345, 343, 848405, 3434, 3434344, 3535, 93994, 2230404, 4334 };

static struct three t[] = {
    i3(879045), i3(394), i3(99405644), i3(33434), i3(232323), i3(4334), i3(5454), i3(343),
    i3(45545), i3(454), i3(324), i3(22), i3(34344), i3(233), i3(45345), i3(343),
    i3(848405), i3(3434), i3(3434344), i3(3535), i3(93994), i3(2230404), i3(4334)
};

int main(void) {
    int i;

    /* String sort */
    qsort(s, sizeof(s) / sizeof(s[0]), sizeof(s[0]), scmp);
    for (i = 0; i < (int)(sizeof(s) / sizeof(char *) - 1); i++) {
        if (strcmp(s[i], s[i + 1]) > 0) {
            err++;
            break;
        }
    }

    /* Integer sort */
    qsort(n, sizeof(n) / sizeof(n[0]), sizeof(n[0]), icmp);
    for (i = 0; i < (int)(sizeof(n) / sizeof(n[0]) - 1); i++) {
        if (n[i] > n[i + 1]) {
            err++;
            break;
        }
    }

    /* Three byte struct sort */
    qsort(t, sizeof(t) / sizeof(t[0]), sizeof(t[0]), tcmp);
    for (i = 0; i < (int)(sizeof(t) / sizeof(t[0]) - 1); i++) {
        if (tcmp(&t[i], &t[i + 1]) > 0) {
            err++;
            break;
        }
    }

    return err;
}
