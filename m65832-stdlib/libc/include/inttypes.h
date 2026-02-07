#ifndef _INTTYPES_H
#define _INTTYPES_H

#include <stdint.h>

/* Format specifiers for printf/scanf */
#define PRId8   "d"
#define PRId16  "d"
#define PRId32  "d"
#define PRId64  "lld"
#define PRIi8   "i"
#define PRIi16  "i"
#define PRIi32  "i"
#define PRIi64  "lli"
#define PRIu8   "u"
#define PRIu16  "u"
#define PRIu32  "u"
#define PRIu64  "llu"
#define PRIx8   "x"
#define PRIx16  "x"
#define PRIx32  "x"
#define PRIx64  "llx"
#define PRIX8   "X"
#define PRIX16  "X"
#define PRIX32  "X"
#define PRIX64  "llX"
#define PRIo8   "o"
#define PRIo16  "o"
#define PRIo32  "o"
#define PRIo64  "llo"

#define SCNd8   "hhd"
#define SCNd16  "hd"
#define SCNd32  "d"
#define SCNd64  "lld"
#define SCNu8   "hhu"
#define SCNu16  "hu"
#define SCNu32  "u"
#define SCNu64  "llu"

#define PRIdPTR  PRId32
#define PRIuPTR  PRIu32
#define PRIxPTR  PRIx32

/* Integer division */
typedef struct { int quot; int rem; } imaxdiv_t;

intmax_t imaxabs(intmax_t j);
imaxdiv_t imaxdiv(intmax_t numer, intmax_t denom);
intmax_t strtoimax(const char *nptr, char **endptr, int base);
uintmax_t strtoumax(const char *nptr, char **endptr, int base);

#endif /* _INTTYPES_H */
