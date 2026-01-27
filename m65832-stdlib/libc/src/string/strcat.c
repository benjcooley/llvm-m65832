/* strcat.c */
#include <string.h>

char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;
    while ((*d++ = *src++) != '\0')
        ;
    return dest;
}

char *strncat(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (*d) d++;
    while (n-- && (*d++ = *src++) != '\0')
        ;
    if (n == (size_t)-1) *d = '\0';
    return dest;
}
