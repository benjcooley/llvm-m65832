/* stdio.h - Standard I/O */

#ifndef _STDIO_H
#define _STDIO_H

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EOF (-1)

/* Simple output functions - no FILE* for minimal implementation */
int putchar(int c);
int puts(const char *s);
int printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
int sprintf(char *str, const char *format, ...) __attribute__((format(printf, 2, 3)));
int snprintf(char *str, size_t size, const char *format, ...) __attribute__((format(printf, 3, 4)));
int vprintf(const char *format, va_list ap);
int vsprintf(char *str, const char *format, va_list ap);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);

/* Simple input functions */
int getchar(void);
char *gets(char *s);  /* deprecated but simple */

#ifdef __cplusplus
}
#endif

#endif /* _STDIO_H */
