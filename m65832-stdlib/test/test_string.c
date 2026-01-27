/* test_string.c - Comprehensive string.h tests */
#include <string.h>

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name, condition) do { \
    if (condition) { tests_passed++; } \
    else { tests_failed++; } \
} while(0)

/* Test strlen */
void test_strlen(void) {
    TEST("strlen empty", strlen("") == 0);
    TEST("strlen hello", strlen("Hello") == 5);
    TEST("strlen single", strlen("X") == 1);
    TEST("strlen spaces", strlen("  ") == 2);
}

/* Test strcpy */
void test_strcpy(void) {
    char buf[32];
    
    strcpy(buf, "Hello");
    TEST("strcpy basic", strcmp(buf, "Hello") == 0);
    
    strcpy(buf, "");
    TEST("strcpy empty", buf[0] == '\0');
    
    strcpy(buf, "World");
    TEST("strcpy overwrite", strcmp(buf, "World") == 0);
}

/* Test strncpy */
void test_strncpy(void) {
    char buf[32];
    
    /* Initialize buffer */
    memset(buf, 'X', sizeof(buf));
    
    strncpy(buf, "Hello", 10);
    TEST("strncpy basic", strcmp(buf, "Hello") == 0);
    TEST("strncpy pads", buf[5] == '\0' && buf[6] == '\0');
    
    strncpy(buf, "TooLongString", 5);
    TEST("strncpy truncate", buf[0] == 'T' && buf[4] == 'o');
}

/* Test strcat */
void test_strcat(void) {
    char buf[32];
    
    strcpy(buf, "Hello");
    strcat(buf, " World");
    TEST("strcat basic", strcmp(buf, "Hello World") == 0);
    
    strcpy(buf, "");
    strcat(buf, "Test");
    TEST("strcat to empty", strcmp(buf, "Test") == 0);
    
    strcpy(buf, "Foo");
    strcat(buf, "");
    TEST("strcat empty", strcmp(buf, "Foo") == 0);
}

/* Test strcmp */
void test_strcmp(void) {
    TEST("strcmp equal", strcmp("abc", "abc") == 0);
    TEST("strcmp less", strcmp("abc", "abd") < 0);
    TEST("strcmp greater", strcmp("abd", "abc") > 0);
    TEST("strcmp empty", strcmp("", "") == 0);
    TEST("strcmp prefix", strcmp("ab", "abc") < 0);
    TEST("strcmp longer", strcmp("abc", "ab") > 0);
}

/* Test strncmp */
void test_strncmp(void) {
    TEST("strncmp equal n", strncmp("abcdef", "abcxyz", 3) == 0);
    TEST("strncmp diff", strncmp("abcdef", "abcxyz", 4) < 0);
    TEST("strncmp zero n", strncmp("abc", "xyz", 0) == 0);
    TEST("strncmp full", strncmp("abc", "abc", 100) == 0);
}

/* Test strchr */
void test_strchr(void) {
    const char *s = "Hello World";
    
    TEST("strchr found", strchr(s, 'o') == s + 4);
    TEST("strchr first", strchr(s, 'H') == s);
    TEST("strchr last", strchr(s, 'd') == s + 10);
    TEST("strchr not found", strchr(s, 'x') == 0);
    TEST("strchr null term", strchr(s, '\0') == s + 11);
}

/* Test strrchr */
void test_strrchr(void) {
    const char *s = "Hello World";
    
    TEST("strrchr last o", strrchr(s, 'o') == s + 7);
    TEST("strrchr first", strrchr(s, 'H') == s);
    TEST("strrchr not found", strrchr(s, 'x') == 0);
}

/* Test memcpy */
void test_memcpy(void) {
    char src[] = "Hello World";
    char dst[32];
    
    memset(dst, 0, sizeof(dst));
    memcpy(dst, src, 5);
    TEST("memcpy partial", dst[0] == 'H' && dst[4] == 'o' && dst[5] == '\0');
    
    memcpy(dst, src, 12);
    TEST("memcpy full", strcmp(dst, "Hello World") == 0);
    
    memcpy(dst, src, 0);
    TEST("memcpy zero", dst[0] == 'H'); /* unchanged */
}

/* Test memmove - handles overlapping regions */
void test_memmove(void) {
    char buf[32] = "Hello World";
    
    /* Non-overlapping */
    memmove(buf + 20, buf, 5);
    TEST("memmove non-overlap", buf[20] == 'H');
    
    /* Overlapping - forward */
    strcpy(buf, "0123456789");
    memmove(buf + 2, buf, 5);
    TEST("memmove overlap fwd", buf[2] == '0' && buf[6] == '4');
    
    /* Overlapping - backward */
    strcpy(buf, "0123456789");
    memmove(buf, buf + 2, 5);
    TEST("memmove overlap bwd", buf[0] == '2' && buf[4] == '6');
}

/* Test memset */
void test_memset(void) {
    char buf[32];
    
    memset(buf, 'A', 10);
    buf[10] = '\0';
    TEST("memset fill", buf[0] == 'A' && buf[9] == 'A');
    
    memset(buf, 0, 5);
    TEST("memset zero", buf[0] == 0 && buf[4] == 0 && buf[5] == 'A');
}

/* Test memcmp */
void test_memcmp(void) {
    TEST("memcmp equal", memcmp("abc", "abc", 3) == 0);
    TEST("memcmp less", memcmp("abc", "abd", 3) < 0);
    TEST("memcmp greater", memcmp("abd", "abc", 3) > 0);
    TEST("memcmp partial", memcmp("abcdef", "abcxyz", 3) == 0);
    TEST("memcmp zero", memcmp("abc", "xyz", 0) == 0);
}

/* Test memchr */
void test_memchr(void) {
    const char *s = "Hello\0World";
    
    TEST("memchr found", memchr(s, 'e', 11) == s + 1);
    TEST("memchr null", memchr(s, '\0', 11) == s + 5);
    TEST("memchr past null", memchr(s, 'W', 11) == s + 6);
    TEST("memchr not found", memchr(s, 'x', 11) == 0);
}

/* Test strstr */
void test_strstr(void) {
    const char *s = "Hello World";
    
    TEST("strstr found", strstr(s, "World") == s + 6);
    TEST("strstr start", strstr(s, "Hello") == s);
    TEST("strstr not found", strstr(s, "xyz") == 0);
    TEST("strstr empty needle", strstr(s, "") == s);
    TEST("strstr single char", strstr(s, "o") == s + 4);
}

int main(void) {
    test_strlen();
    test_strcpy();
    test_strncpy();
    test_strcat();
    test_strcmp();
    test_strncmp();
    test_strchr();
    test_strrchr();
    test_memcpy();
    test_memmove();
    test_memset();
    test_memcmp();
    test_memchr();
    test_strstr();
    
    /* Return 0 if all tests passed, otherwise number of failures */
    return tests_failed;
}
