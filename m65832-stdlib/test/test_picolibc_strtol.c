/* test_picolibc_strtol.c - Adapted picolibc strtol tests for M65832
 * Based on picolibc libc-testsuite/strtol.c
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Test result tracking */
static int err = 0;

#define TEST(r, f, x) do { \
    (r) = (f); \
    if ((r) != (x)) { err++; } \
} while (0)

int main(void) {
    int i;
    long l;
    unsigned long ul;
    char *s, *c;

    /* Basic tests */
    TEST(l, atol("2147483647"), 2147483647L);
    TEST(l, strtol("2147483647", 0, 0), 2147483647L);
    TEST(ul, strtoul("4294967295", 0, 0), 4294967295UL);

    /* Our M65832 has 32-bit long */
    /* Overflow tests */
    errno = 0;
    TEST(l, strtol(s = "2147483648", &c, 0), 2147483647L);
    if (c - s != 10) err++;
    if (errno != ERANGE) err++;
    
    errno = 0;
    TEST(l, strtol(s = "-2147483649", &c, 0), -2147483647L - 1);
    if (c - s != 11) err++;
    if (errno != ERANGE) err++;
    
    errno = 0;
    TEST(ul, strtoul(s = "4294967296", &c, 0), 4294967295UL);
    if (c - s != 10) err++;
    if (errno != ERANGE) err++;
    
    errno = 0;
    TEST(ul, strtoul(s = "-1", &c, 0), -1UL);
    if (c - s != 2) err++;
    if (errno != 0) err++;
    
    errno = 0;
    TEST(ul, strtoul(s = "-2", &c, 0), -2UL);
    if (c - s != 2) err++;
    if (errno != 0) err++;

    /* Base conversions */
    TEST(l, strtol("z", 0, 36), 35);
    TEST(l, strtol("00010010001101000101011001111000", 0, 2), 0x12345678L);
    TEST(l, strtol(s = "0F5F", &c, 16), 0x0f5f);

    TEST(l, strtol(s = "0xz", &c, 16), 0);
    if (c - s != 1) err++;

    TEST(l, strtol(s = "0x1234", &c, 16), 0x1234);
    if (c - s != 6) err++;

    /* Invalid base */
    errno = 0;
    c = 0;
    TEST(l, strtol(s = "123", &c, 37), 0);
    if (errno != EINVAL) err++;

    /* Octal */
    TEST(l, strtol(s = "  15437", &c, 8), 015437);
    if (c - s != 7) err++;

    /* Leading whitespace */
    TEST(l, strtol(s = "  1", &c, 0), 1);
    if (c - s != 3) err++;

    /* More negative tests */
    TEST(l, strtol("-123", 0, 10), -123);
    TEST(l, strtol("   -456", 0, 10), -456);
    TEST(l, strtol("+789", 0, 10), 789);

    /* Hex with 0x prefix */
    TEST(l, strtol("0x100", 0, 0), 256);
    TEST(l, strtol("0X200", 0, 0), 512);
    TEST(l, strtol("-0xFF", 0, 0), -255);

    /* Octal with 0 prefix */
    TEST(l, strtol("0100", 0, 0), 64);
    TEST(l, strtol("-0100", 0, 0), -64);

    return err;
}
