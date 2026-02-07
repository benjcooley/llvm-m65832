/* strncpy - copy at most n bytes from src to dest, zero-pad remainder */
#include <string.h>

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    /* Copy from src up to n characters */
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    /* Zero-fill remainder */
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}
