/* test_printf.c - Test sprintf/snprintf formatting */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name, condition) do { \
    if (condition) { tests_passed++; } \
    else { tests_failed++; } \
} while(0)

/* Test basic sprintf */
void test_sprintf_basic(void) {
    char buf[128];
    
    /* Simple string */
    sprintf(buf, "Hello");
    TEST("sprintf simple", strcmp(buf, "Hello") == 0);
    
    /* String format */
    sprintf(buf, "Hello %s", "World");
    TEST("sprintf %s", strcmp(buf, "Hello World") == 0);
    
    /* Character format */
    sprintf(buf, "Char: %c", 'X');
    TEST("sprintf %c", strcmp(buf, "Char: X") == 0);
    
    /* Percent literal */
    sprintf(buf, "100%%");
    TEST("sprintf %%", strcmp(buf, "100%") == 0);
}

/* Test integer formatting */
void test_sprintf_int(void) {
    char buf[128];
    
    /* Decimal */
    sprintf(buf, "%d", 42);
    TEST("sprintf %d positive", strcmp(buf, "42") == 0);
    
    sprintf(buf, "%d", -123);
    TEST("sprintf %d negative", strcmp(buf, "-123") == 0);
    
    sprintf(buf, "%d", 0);
    TEST("sprintf %d zero", strcmp(buf, "0") == 0);
    
    /* Unsigned */
    sprintf(buf, "%u", 4294967295U);
    TEST("sprintf %u max", strcmp(buf, "4294967295") == 0);
    
    /* Hex */
    sprintf(buf, "%x", 255);
    TEST("sprintf %x lower", strcmp(buf, "ff") == 0);
    
    sprintf(buf, "%X", 255);
    TEST("sprintf %X upper", strcmp(buf, "FF") == 0);
    
    sprintf(buf, "0x%08x", 0xDEADBEEF);
    TEST("sprintf %08x pad", strcmp(buf, "0xdeadbeef") == 0);
    
    /* Octal */
    sprintf(buf, "%o", 63);
    TEST("sprintf %o", strcmp(buf, "77") == 0);
}

/* Test width and padding */
void test_sprintf_width(void) {
    char buf[128];
    
    /* Right-aligned (default) */
    sprintf(buf, "[%5d]", 42);
    TEST("sprintf width right", strcmp(buf, "[   42]") == 0);
    
    /* Left-aligned */
    sprintf(buf, "[%-5d]", 42);
    TEST("sprintf width left", strcmp(buf, "[42   ]") == 0);
    
    /* Zero-padded */
    sprintf(buf, "[%05d]", 42);
    TEST("sprintf zero pad", strcmp(buf, "[00042]") == 0);
    
    /* Zero-padded negative */
    sprintf(buf, "[%05d]", -42);
    TEST("sprintf zero pad neg", strcmp(buf, "[-0042]") == 0);
    
    /* String width */
    sprintf(buf, "[%10s]", "Hi");
    TEST("sprintf str width", strcmp(buf, "[        Hi]") == 0);
    
    sprintf(buf, "[%-10s]", "Hi");
    TEST("sprintf str width left", strcmp(buf, "[Hi        ]") == 0);
}

/* Test precision */
void test_sprintf_precision(void) {
    char buf[128];
    
    /* String precision (max length) */
    sprintf(buf, "%.5s", "Hello World");
    TEST("sprintf str prec", strcmp(buf, "Hello") == 0);
    
    /* Integer precision (min digits) */
    sprintf(buf, "%.5d", 42);
    TEST("sprintf int prec", strcmp(buf, "00042") == 0);
    
    /* Zero precision, zero value */
    sprintf(buf, "[%.0d]", 0);
    TEST("sprintf zero prec zero", strcmp(buf, "[]") == 0);
}

/* Test long integers */
void test_sprintf_long(void) {
    char buf[128];
    
    sprintf(buf, "%ld", 2000000000L);
    TEST("sprintf %ld", strcmp(buf, "2000000000") == 0);
    
    sprintf(buf, "%ld", -2000000000L);
    TEST("sprintf %ld neg", strcmp(buf, "-2000000000") == 0);
    
    sprintf(buf, "%lu", 4000000000UL);
    TEST("sprintf %lu", strcmp(buf, "4000000000") == 0);
}

/* Test snprintf bounds checking */
void test_snprintf(void) {
    char buf[10];
    int ret;
    
    /* Fits in buffer */
    memset(buf, 'X', sizeof(buf));
    ret = snprintf(buf, sizeof(buf), "Hi");
    TEST("snprintf fits", strcmp(buf, "Hi") == 0 && ret == 2);
    
    /* Truncated */
    memset(buf, 'X', sizeof(buf));
    ret = snprintf(buf, sizeof(buf), "Hello World!");
    TEST("snprintf truncated", strncmp(buf, "Hello Wor", 9) == 0);
    TEST("snprintf null term", buf[9] == '\0');
    TEST("snprintf ret full", ret == 12);  /* would have written 12 chars */
    
    /* Zero size */
    memset(buf, 'X', sizeof(buf));
    ret = snprintf(buf, 0, "Test");
    TEST("snprintf zero size", buf[0] == 'X');  /* unchanged */
    TEST("snprintf zero ret", ret == 4);
}

/* Test multiple arguments */
void test_sprintf_multi(void) {
    char buf[128];
    
    sprintf(buf, "%s=%d", "answer", 42);
    TEST("sprintf multi 1", strcmp(buf, "answer=42") == 0);
    
    sprintf(buf, "%d+%d=%d", 1, 2, 3);
    TEST("sprintf multi 2", strcmp(buf, "1+2=3") == 0);
    
    sprintf(buf, "[%c][%s][%d][%x]", 'A', "BC", 123, 255);
    TEST("sprintf multi 3", strcmp(buf, "[A][BC][123][ff]") == 0);
}

/* Test float formatting (va_arg for double) */
void test_sprintf_float(void) {
    char buf[128];

    sprintf(buf, "%.6f", 1.23);
    TEST("sprintf %f 1.23", strstr(buf, "1.23") != NULL);

    sprintf(buf, "%.6f", 12312.1);
    TEST("sprintf %f 12312.1", strstr(buf, "12312") != NULL);

    sprintf(buf, "foo %f %f %f %f", 1.23, 12312.1, 3.1, 13.1);
    TEST("sprintf %f x4", strstr(buf, "1.23") != NULL && strstr(buf, "12312") != NULL);
}

/* Test pointer formatting */
void test_sprintf_pointer(void) {
    char buf[128];
    void *p = (void *)0x12345678;
    
    sprintf(buf, "%p", p);
    /* Result format may vary, but should contain the address */
    TEST("sprintf %p contains addr", strstr(buf, "12345678") != NULL ||
                                     strstr(buf, "0x12345678") != NULL);
}

int main(void) {
    test_sprintf_basic();
    test_sprintf_int();
    test_sprintf_width();
    test_sprintf_precision();
    test_sprintf_long();
    test_sprintf_float();
    test_snprintf();
    test_sprintf_multi();
    test_sprintf_pointer();
    
    /* Return 0 if all tests passed, otherwise number of failures */
    return tests_failed;
}
