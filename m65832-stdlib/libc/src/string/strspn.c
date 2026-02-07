/* strspn.c - String span functions */
#include <string.h>

size_t strspn(const char *s, const char *accept) {
    const char *p = s;
    while (*p) {
        const char *a = accept;
        int found = 0;
        while (*a) {
            if (*p == *a) { found = 1; break; }
            a++;
        }
        if (!found) break;
        p++;
    }
    return (size_t)(p - s);
}

size_t strcspn(const char *s, const char *reject) {
    const char *p = s;
    while (*p) {
        const char *r = reject;
        while (*r) {
            if (*p == *r) return (size_t)(p - s);
            r++;
        }
        p++;
    }
    return (size_t)(p - s);
}

char *strpbrk(const char *s, const char *accept) {
    while (*s) {
        const char *a = accept;
        while (*a) {
            if (*s == *a) return (char *)s;
            a++;
        }
        s++;
    }
    return NULL;
}

char *strtok(char *str, const char *delim) {
    static char *last;
    if (str) last = str;
    if (!last) return NULL;
    
    /* Skip leading delimiters */
    last += strspn(last, delim);
    if (*last == '\0') { last = NULL; return NULL; }
    
    char *token = last;
    last = last + strcspn(last, delim);
    if (*last) {
        *last = '\0';
        last++;
    } else {
        last = NULL;
    }
    return token;
}

void *memset_explicit(void *s, int c, size_t n) {
    volatile unsigned char *p = (volatile unsigned char *)s;
    size_t i;
    for (i = 0; i < n; i++) {
        p[i] = (unsigned char)c;
    }
    return s;
}
