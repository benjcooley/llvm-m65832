/* memrchr.c */
#include <string.h>

void *memrchr(const void *s, int c, size_t n) {
    const unsigned char *p = (const unsigned char *)s + n;
    unsigned char val = (unsigned char)c;
    size_t i;
    for (i = 0; i < n; i++) {
        --p;
        if (*p == val) return (void *)p;
    }
    return NULL;
}
