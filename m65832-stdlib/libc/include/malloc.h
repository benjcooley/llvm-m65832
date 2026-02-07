/* malloc.h - Memory allocation (non-standard, for picolibc compat) */

#ifndef _MALLOC_H
#define _MALLOC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void *memalign(size_t alignment, size_t size);
int posix_memalign(void **memptr, size_t alignment, size_t size);

#ifdef __cplusplus
}
#endif


void *reallocarray(void *ptr, size_t nmemb, size_t size);

#endif /* _MALLOC_H */
