/* test_picolibc_string.c - Adapted picolibc string tests for M65832
 * Based on picolibc libc-testsuite/string.c but modified to work
 * without printf (uses exit code for pass/fail reporting)
 */

#include <string.h>

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(r, f, x) do { \
    (r) = (f); \
    if ((r) != (x)) { tests_failed++; } \
    else { tests_passed++; } \
} while (0)

#define TEST_S(s, x) do { \
    if (s == NULL || strcmp((s), (x)) != 0) { tests_failed++; } \
    else { tests_passed++; } \
} while (0)

int main(void) {
    char b[32] = {0};
    char c[32] = {0};
    char *s = NULL;
    int i;

    /* strcpy tests */
    c[0] = 'a'; c[1] = 'b'; c[2] = 'c'; c[3] = 0;
    
    TEST(s, strcpy(b, c), b);
    TEST_S(s, "abc");
    TEST(s, strcpy(b + 1, c), b + 1);
    TEST_S(s, "abc");
    TEST(s, strcpy(b + 2, c), b + 2);
    TEST_S(s, "abc");
    TEST(s, strcpy(b + 3, c), b + 3);
    TEST_S(s, "abc");
    
    TEST(s, strcpy(b + 1, c + 1), b + 1);
    TEST_S(s, "bc");
    TEST(s, strcpy(b + 2, c + 2), b + 2);
    TEST_S(s, "c");
    TEST(s, strcpy(b + 3, c + 3), b + 3);
    TEST_S(s, "");

    /* strncpy tests */
    TEST(s, memset(b, 'x', sizeof b), b);
    TEST(s, strncpy(b, "abc", sizeof b - 1), b);
    TEST(i, memcmp(b, "abc\0\0\0\0", 8), 0);
    TEST(i, b[sizeof b - 1], 'x');

    b[3] = 'x';
    b[4] = 0;
    strncpy(b, "abc", 3);
    TEST(i, b[2], 'c');
    TEST(i, b[3], 'x');

    /* strncmp tests */
    TEST(i, !strncmp("abcd", "abce", 3), 1);
    TEST(i, !!strncmp("abc", "abd", 3), 1);

    /* strncat tests */
    strcpy(b, "abc");
    TEST(s, strncat(b, "123456", 3), b);
    TEST(i, b[6], 0);
    TEST_S(s, "abc123");

    /* strchr/strrchr/strspn/strcspn/strpbrk tests */
    strcpy(b, "aaababccdd0001122223");
    TEST(s, strchr(b, 'b'), b + 3);
    TEST(s, strrchr(b, 'b'), b + 5);
    TEST(i, strspn(b, "abcd"), 10);
    TEST(i, strcspn(b, "0123"), 10);
    TEST(s, strpbrk(b, "0123"), b + 10);

    /* strtok tests */
    strcpy(b, "abc   123; xyz; foo");
    TEST(s, strtok(b, " "), b);
    TEST_S(s, "abc");

    TEST(s, strtok(NULL, ";"), b + 4);
    TEST_S(s, "  123");

    TEST(s, strtok(NULL, " ;"), b + 11);
    TEST_S(s, "xyz");

    TEST(s, strtok(NULL, " ;"), b + 16);
    TEST_S(s, "foo");

    /* Return 0 if all tests passed, otherwise number of failures */
    return tests_failed;
}
