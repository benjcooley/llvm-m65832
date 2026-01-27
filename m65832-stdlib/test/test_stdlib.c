/* test_stdlib.c - Comprehensive stdlib.h tests */
#include <stdlib.h>
#include <string.h>

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name, condition) do { \
    if (condition) { tests_passed++; } \
    else { tests_failed++; } \
} while(0)

/* Test abs */
void test_abs(void) {
    TEST("abs positive", abs(5) == 5);
    TEST("abs negative", abs(-5) == 5);
    TEST("abs zero", abs(0) == 0);
    TEST("abs large", abs(1000000) == 1000000);
    TEST("abs large neg", abs(-1000000) == 1000000);
}

/* Test labs */
void test_labs(void) {
    TEST("labs positive", labs(5L) == 5L);
    TEST("labs negative", labs(-5L) == 5L);
    TEST("labs zero", labs(0L) == 0L);
    TEST("labs large", labs(2000000000L) == 2000000000L);
}

/* Test atoi */
void test_atoi(void) {
    TEST("atoi positive", atoi("123") == 123);
    TEST("atoi negative", atoi("-456") == -456);
    TEST("atoi zero", atoi("0") == 0);
    TEST("atoi leading space", atoi("  42") == 42);
    TEST("atoi trailing", atoi("99abc") == 99);
    TEST("atoi plus sign", atoi("+77") == 77);
    TEST("atoi empty", atoi("") == 0);
    TEST("atoi letters", atoi("abc") == 0);
}

/* Test atol */
void test_atol(void) {
    TEST("atol positive", atol("123456") == 123456L);
    TEST("atol negative", atol("-789012") == -789012L);
    TEST("atol large", atol("2000000000") == 2000000000L);
}

/* Test strtol */
void test_strtol(void) {
    char *end;
    
    TEST("strtol decimal", strtol("123", &end, 10) == 123);
    TEST("strtol negative", strtol("-456", &end, 10) == -456);
    TEST("strtol hex", strtol("ff", &end, 16) == 255);
    TEST("strtol hex 0x", strtol("0xff", &end, 16) == 255);
    TEST("strtol octal", strtol("77", &end, 8) == 63);
    TEST("strtol binary", strtol("1010", &end, 2) == 10);
    TEST("strtol auto hex", strtol("0x10", &end, 0) == 16);
    TEST("strtol auto oct", strtol("010", &end, 0) == 8);
    TEST("strtol auto dec", strtol("10", &end, 0) == 10);
    
    /* Check endptr */
    strtol("123abc", &end, 10);
    TEST("strtol endptr", *end == 'a');
    
    strtol("   -42xyz", &end, 10);
    TEST("strtol space end", *end == 'x');
}

/* Test strtoul */
void test_strtoul(void) {
    char *end;
    
    TEST("strtoul positive", strtoul("123", &end, 10) == 123UL);
    TEST("strtoul hex", strtoul("DEADBEEF", &end, 16) == 0xDEADBEEFUL);
    TEST("strtoul large", strtoul("4000000000", &end, 10) == 4000000000UL);
}

/* Test malloc and free */
void test_malloc(void) {
    void *p1 = malloc(100);
    TEST("malloc returns non-null", p1 != 0);
    
    /* Write to allocated memory */
    if (p1) {
        memset(p1, 0xAA, 100);
        TEST("malloc writeable", ((char*)p1)[0] == (char)0xAA);
        ((char*)p1)[99] = 0x55;
        TEST("malloc end writeable", ((char*)p1)[99] == 0x55);
    }
    
    /* Multiple allocations */
    void *p2 = malloc(50);
    void *p3 = malloc(200);
    TEST("malloc multiple", p2 != 0 && p3 != 0);
    TEST("malloc different", p1 != p2 && p2 != p3 && p1 != p3);
    
    /* Free (may be no-op in bump allocator) */
    free(p1);
    free(p2);
    free(p3);
}

/* Test calloc */
void test_calloc(void) {
    int *arr = (int *)calloc(10, sizeof(int));
    TEST("calloc returns non-null", arr != 0);
    
    if (arr) {
        /* Check zeroed */
        int all_zero = 1;
        for (int i = 0; i < 10; i++) {
            if (arr[i] != 0) all_zero = 0;
        }
        TEST("calloc zeroed", all_zero);
        
        free(arr);
    }
}

/* Test realloc */
void test_realloc(void) {
    /* Allocate initial block */
    char *p = (char *)malloc(10);
    TEST("realloc initial", p != 0);
    
    if (p) {
        /* Write some data */
        strcpy(p, "Hello");
        
        /* Reallocate larger */
        char *p2 = (char *)realloc(p, 100);
        TEST("realloc larger", p2 != 0);
        
        if (p2) {
            /* Check data preserved */
            TEST("realloc preserves", strcmp(p2, "Hello") == 0);
            free(p2);
        }
    }
    
    /* realloc NULL is like malloc */
    void *p3 = realloc((void*)0, 50);
    TEST("realloc null", p3 != 0);
    if (p3) free(p3);
}

int main(void) {
    test_abs();
    test_labs();
    test_atoi();
    test_atol();
    test_strtol();
    test_strtoul();
    test_malloc();
    test_calloc();
    test_realloc();
    
    /* Return 0 if all tests passed, otherwise number of failures */
    return tests_failed;
}
