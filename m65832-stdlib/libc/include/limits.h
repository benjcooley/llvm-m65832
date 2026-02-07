#ifndef _LIMITS_H
#define _LIMITS_H

/* char */
#define CHAR_BIT    8
#define SCHAR_MIN   (-128)
#define SCHAR_MAX   127
#define UCHAR_MAX   255
#define CHAR_MIN    SCHAR_MIN
#define CHAR_MAX    SCHAR_MAX

/* short (16-bit) */
#define SHRT_MIN    (-32768)
#define SHRT_MAX    32767
#define USHRT_MAX   65535

/* int (32-bit on M65832) */
#define INT_MIN     (-2147483647 - 1)
#define INT_MAX     2147483647
#define UINT_MAX    4294967295U

/* long (32-bit on M65832) */
#define LONG_MIN    (-2147483647L - 1L)
#define LONG_MAX    2147483647L
#define ULONG_MAX   4294967295UL

/* long long (64-bit) */
#define LLONG_MIN   (-9223372036854775807LL - 1LL)
#define LLONG_MAX   9223372036854775807LL
#define ULLONG_MAX  18446744073709551615ULL

/* POSIX additions */
#define SSIZE_MAX   LONG_MAX
#define PATH_MAX    256
#define NAME_MAX    255

/* Minimum values for maximum magnitude */
#define MB_LEN_MAX  4

/* INT16/INT32 macros for picolibc test detection */
#define INT16_MAX   32767
#define INT16_MIN   (-32768)

#endif /* _LIMITS_H */
