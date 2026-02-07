/* wctype.c - Minimal wide character type functions (ASCII only) */
#include <wctype.h>
#include <ctype.h>

int iswalpha(wint_t wc) { return (wc < 128) ? isalpha((int)wc) : 0; }
int iswdigit(wint_t wc) { return (wc < 128) ? isdigit((int)wc) : 0; }
int iswspace(wint_t wc) { return (wc < 128) ? isspace((int)wc) : 0; }
int iswupper(wint_t wc) { return (wc < 128) ? isupper((int)wc) : 0; }
int iswlower(wint_t wc) { return (wc < 128) ? islower((int)wc) : 0; }
int iswprint(wint_t wc) { return (wc < 128) ? isprint((int)wc) : 0; }
int iswpunct(wint_t wc) { return (wc < 128) ? ispunct((int)wc) : 0; }
int iswcntrl(wint_t wc) { return (wc < 128) ? iscntrl((int)wc) : 0; }
int iswalnum(wint_t wc) { return (wc < 128) ? isalnum((int)wc) : 0; }
int iswgraph(wint_t wc) { return (wc < 128) ? isgraph((int)wc) : 0; }
int iswxdigit(wint_t wc) { return (wc < 128) ? isxdigit((int)wc) : 0; }
wint_t towupper(wint_t wc) { return (wc < 128) ? (wint_t)toupper((int)wc) : wc; }
wint_t towlower(wint_t wc) { return (wc < 128) ? (wint_t)tolower((int)wc) : wc; }

wctype_t wctype(const char *property) {
    (void)property;
    return 0;
}

int iswctype(wint_t wc, wctype_t desc) {
    (void)wc; (void)desc;
    return 0;
}

wctrans_t wctrans(const char *charclass) {
    (void)charclass;
    return 0;
}

wint_t towctrans(wint_t wc, wctrans_t desc) {
    (void)desc;
    return wc;
}

int iswblank(wint_t wc) { return wc == L' ' || wc == L'\t'; }

/* Minimal multibyte/wide character conversion (ASCII only) */
#include <wchar.h>

size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps) {
    (void)ps;
    if (s == (void*)0) return 0;
    if (n == 0) return (size_t)-2;
    unsigned char c = *(const unsigned char *)s;
    if (pwc) *pwc = (wchar_t)c;
    return c ? 1 : 0;
}

size_t wcrtomb(char *s, wint_t wc, mbstate_t *ps) {
    (void)ps;
    if (s == (void*)0) return 1;
    *s = (char)(unsigned char)wc;
    return 1;
}

int mbsinit(const mbstate_t *ps) {
    (void)ps;
    return 1;
}
