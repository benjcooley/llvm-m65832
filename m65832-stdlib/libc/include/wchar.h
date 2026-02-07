#ifndef _WCHAR_H
#define _WCHAR_H

#include <stddef.h>
#include <wctype.h>

/* Minimal wchar support - ASCII only for M65832 */

typedef int mbstate_t;

size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps);
size_t wcrtomb(char *s, wint_t wc, mbstate_t *ps);
int mbsinit(const mbstate_t *ps);

#endif /* _WCHAR_H */
