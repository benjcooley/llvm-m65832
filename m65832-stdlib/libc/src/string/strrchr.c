/* strrchr.c */
#include <string.h>

char *strrchr(const char *s, int c) {
    unsigned char ch = (unsigned char)c;
    const char *last = NULL;
    do {
        if ((unsigned char)*s == ch)
            last = s;
    } while (*s++);
    return (char *)last;
}
