#include <string.h>
void *memmem(const void *haystack, size_t haystacklen,
             const void *needle, size_t needlelen) {
    if (!needlelen) return (void *)haystack;
    if (needlelen > haystacklen) return NULL;
    const char *h = haystack;
    for (size_t i = 0; i <= haystacklen - needlelen; i++) {
        if (!memcmp(h + i, needle, needlelen))
            return (void *)(h + i);
    }
    return NULL;
}
