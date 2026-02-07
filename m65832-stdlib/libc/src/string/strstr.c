/* strstr.c */
#include <string.h>

char *strstr(const char *haystack, const char *needle) {
    if (*needle == '\0') return (char *)haystack;
    
    for (; *haystack; haystack++) {
        if (*haystack == *needle) {
            const char *h = haystack;
            const char *n = needle;
            while (*h && *n && (*h == *n)) {
                h++;
                n++;
            }
            if (*n == '\0')
                return (char *)haystack;
        }
    }
    return NULL;
}
