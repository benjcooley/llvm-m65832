/* test_stdlib_basic.c - Basic stdlib function tests */
#include <stdlib.h>
#include <string.h>

static int test_count = 0;
static int fail_count = 0;

#define TEST(cond) do { test_count++; if (!(cond)) fail_count++; } while(0)

int main(void) {
    /* abs/labs */
    TEST(abs(5) == 5);
    TEST(abs(-5) == 5);
    TEST(abs(0) == 0);
    TEST(labs(1000L) == 1000L);
    TEST(labs(-1000L) == 1000L);
    
    /* atoi */
    TEST(atoi("123") == 123);
    TEST(atoi("-456") == -456);
    TEST(atoi("0") == 0);
    TEST(atoi("  42") == 42);
    TEST(atoi("99abc") == 99);
    TEST(atoi("abc") == 0);
    
    /* strtol */
    char *end;
    TEST(strtol("123", &end, 10) == 123);
    TEST(strtol("-456", &end, 10) == -456);
    TEST(strtol("ff", &end, 16) == 255);
    TEST(strtol("0xff", &end, 16) == 255);
    TEST(strtol("77", &end, 8) == 63);
    TEST(strtol("1010", &end, 2) == 10);
    
    /* malloc - basic test */
    void *p = malloc(100);
    TEST(p != (void*)0);
    if (p) {
        memset(p, 0xAA, 100);
        TEST(((char*)p)[0] == (char)0xAA);
        TEST(((char*)p)[99] == (char)0xAA);
        free(p);
    }
    
    /* calloc */
    int *arr = (int *)calloc(4, sizeof(int));
    TEST(arr != (void*)0);
    if (arr) {
        TEST(arr[0] == 0);
        TEST(arr[3] == 0);
        free(arr);
    }
    
    return fail_count;
}
