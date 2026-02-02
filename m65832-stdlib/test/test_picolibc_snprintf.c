/* test_picolibc_snprintf.c - Adapted picolibc snprintf tests for M65832
 * Based on picolibc libc-testsuite/snprintf.c
 */

#include <stdio.h>
#include <string.h>

static int err = 0;

#define TEST(r, f, x) do { \
    (r) = (f); \
    if ((r) != (x)) { err++; } \
} while (0)

#define TEST_S(s, x) do { \
    if (strcmp((s), (x)) != 0) { err++; } \
} while (0)

static const struct {
    const char *fmt;
    int i;
    const char *expect;
} int_tests[] = {
    /* width, precision, alignment */
    { "%04d",   12,  "0012"  },
    { "%.3d",   12,  "012"   },
    { "%3d",    12,  " 12"   },
    { "%-3d",   12,  "12 "   },
    { "%+3d",   12,  "+12"   },
    { "%+-5d",  12,  "+12  " },
    { "%+- 5d", 12,  "+12  " },
    { "%- 5d",  12,  " 12  " },
    { "% d",    12,  " 12"   },
    { "%0-5d",  12,  "12   " },
    { "%-05d",  12,  "12   " },

    /* ...explicit precision of 0 shall be no characters. */
    { "%.0d",   0,   ""      },
    { "%.0o",   0,   ""      },
    { "%#.0d",  0,   ""      },
    { "%#.0o",  0,   ""      },
    { "%#.0x",  0,   ""      },

    /* ...but it still has to honor width and flags. */
    { "%2.0u",  0,   "  "    },
    { "%02.0u", 0,   "  "    },
    { "%2.0d",  0,   "  "    },
    { "%02.0d", 0,   "  "    },
    { "% .0d",  0,   " "     },
    { "%+.0d",  0,   "+"     },

    /* hex: test alt form and case */
    { "%x",     63,  "3f"    },
    { "%#x",    63,  "0x3f"  },
    { "%X",     63,  "3F"    },

    /* octal: test alt form */
    { "%o",     15,  "17"    },
    { "%#o",    15,  "017"   },

    { 0, 0, 0 }
};

int main(void) {
    int i, j;
    char b[200];

    /* Basic length tests */
    TEST(i, snprintf(0, 0, "%ld", 123456L), 6);
    TEST(i, snprintf(0, 0, "%.4s", "hello"), 4);
    TEST(i, snprintf(b, 0, "%.0s", "goodbye"), 0);

    /* Truncation test */
    strcpy(b, "xxxxxxxx");
    TEST(i, snprintf(b, 4, "%ld", 123456L), 6);
    TEST_S(b, "123");
    if (b[5] != 'x') err++;  /* buffer overrun check */

    /* Integer format tests */
    for (j = 0; int_tests[j].fmt; j++) {
        TEST(i, snprintf(b, sizeof b, int_tests[j].fmt, int_tests[j].i), (int)strlen(b));
        TEST_S(b, int_tests[j].expect);
    }

    /* Additional basic tests */
    snprintf(b, sizeof b, "%d", 12345);
    TEST_S(b, "12345");

    snprintf(b, sizeof b, "%d", -12345);
    TEST_S(b, "-12345");

    snprintf(b, sizeof b, "%u", 4294967295U);
    TEST_S(b, "4294967295");

    snprintf(b, sizeof b, "%08x", 0xdeadbeef);
    TEST_S(b, "deadbeef");

    snprintf(b, sizeof b, "%08X", 0xDEADBEEF);
    TEST_S(b, "DEADBEEF");

    snprintf(b, sizeof b, "Hello %s!", "World");
    TEST_S(b, "Hello World!");

    snprintf(b, sizeof b, "%c%c%c", 'A', 'B', 'C');
    TEST_S(b, "ABC");

    snprintf(b, sizeof b, "%%");
    TEST_S(b, "%");

    /* Long integers */
    snprintf(b, sizeof b, "%ld", 2147483647L);
    TEST_S(b, "2147483647");

    snprintf(b, sizeof b, "%ld", -2147483647L - 1);
    TEST_S(b, "-2147483648");

    snprintf(b, sizeof b, "%lu", 4294967295UL);
    TEST_S(b, "4294967295");

    /* Pointer (format may vary, just check it doesn't crash) */
    i = snprintf(b, sizeof b, "%p", (void *)0x12345678);
    if (i <= 0) err++;

    return err;
}
