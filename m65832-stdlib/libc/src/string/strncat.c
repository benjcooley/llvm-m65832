#include <string.h>
char *strncat(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (*d) d++;
    size_t i;
    for (i = 0; i < n && src[i]; i++)
        *d++ = src[i];
    *d = '\0';
    return dest;
}
