/* stdlib.c - Standard library functions */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Platform hooks - provided by platform layer */
extern void *sys_sbrk(int incr);
extern void sys_exit(int status) __attribute__((noreturn));
extern void sys_abort(void) __attribute__((noreturn));

/*
 * Simple bump allocator - no free support
 * For a real implementation, use a proper allocator
 */
void *malloc(size_t size) {
    if (size == 0) return NULL;
    
    /* Align to 4 bytes */
    size = (size + 3) & ~3;
    
    void *ptr = sys_sbrk(size);
    if (ptr == (void *)-1) {
        return NULL;
    }
    return ptr;
}

void free(void *ptr) {
    /* Simple allocator doesn't support free */
    (void)ptr;
}

void *calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *ptr = malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    /* Simple: allocate new, copy (can't know old size, so this is limited) */
    void *new_ptr = malloc(size);
    if (new_ptr) {
        /* Can't know old size - caller must handle this limitation */
        memcpy(new_ptr, ptr, size);
    }
    return new_ptr;
}

void abort(void) {
    sys_abort();
}

void exit(int status) {
    sys_exit(status);
}

/* atexit - simple implementation */
#define ATEXIT_MAX 32
static void (*atexit_funcs[ATEXIT_MAX])(void);
static int atexit_count = 0;

int atexit(void (*func)(void)) {
    if (atexit_count >= ATEXIT_MAX) {
        return -1;
    }
    atexit_funcs[atexit_count++] = func;
    return 0;
}

/* Called by crt0 before exit */
void __call_atexit(void) {
    while (atexit_count > 0) {
        atexit_funcs[--atexit_count]();
    }
}

int abs(int j) {
    return j < 0 ? -j : j;
}

long labs(long j) {
    return j < 0 ? -j : j;
}

int atoi(const char *nptr) {
    return (int)strtol(nptr, NULL, 10);
}

long atol(const char *nptr) {
    return strtol(nptr, NULL, 10);
}

long strtol(const char *nptr, char **endptr, int base) {
    const char *s = nptr;
    long result = 0;
    int neg = 0;
    
    /* Skip whitespace */
    while (isspace(*s)) s++;
    
    /* Handle sign */
    if (*s == '-') {
        neg = 1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    
    /* Handle base prefix */
    if (base == 0 || base == 16) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            base = 16;
            s += 2;
        } else if (base == 0) {
            base = (s[0] == '0') ? 8 : 10;
        }
    }
    
    /* Convert */
    while (*s) {
        int digit;
        if (isdigit(*s)) {
            digit = *s - '0';
        } else if (isalpha(*s)) {
            digit = tolower(*s) - 'a' + 10;
        } else {
            break;
        }
        
        if (digit >= base) break;
        result = result * base + digit;
        s++;
    }
    
    if (endptr) *endptr = (char *)s;
    return neg ? -result : result;
}

unsigned long strtoul(const char *nptr, char **endptr, int base) {
    return (unsigned long)strtol(nptr, endptr, base);
}
