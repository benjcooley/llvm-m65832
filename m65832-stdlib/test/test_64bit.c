/* test_64bit.c - Test 64-bit integer operations
 * These require library support (__muldi3, __divdi3, etc.) on 32-bit
 */

#include <stdint.h>

static int err = 0;

#define TEST(cond) do { if (!(cond)) err++; } while (0)

/* Test 64-bit basic operations */
void test_64bit_basic(void) {
    int64_t a = 0x123456789ABCDEF0LL;
    int64_t b = 0x0FEDCBA987654321LL;
    
    /* Check value storage */
    TEST((a >> 32) == 0x12345678LL);
    TEST((a & 0xFFFFFFFFLL) == 0x9ABCDEF0LL);
    
    /* Addition */
    int64_t sum = a + b;
    TEST(sum == 0x2222222222222211LL);
    
    /* Subtraction */
    int64_t diff = a - b;
    TEST(diff == 0x02468ACF13579BCFLL);
    
    /* Negation */
    int64_t neg = -a;
    TEST(neg == -0x123456789ABCDEF0LL);
}

/* Test 64-bit comparison */
void test_64bit_compare(void) {
    int64_t a = 0x100000000LL;  /* 4GB - larger than 32-bit max */
    int64_t b = 0x200000000LL;  /* 8GB */
    int64_t c = 0x100000000LL;
    
    TEST(a < b);
    TEST(b > a);
    TEST(a == c);
    TEST(a != b);
    TEST(a <= c);
    TEST(a >= c);
    
    /* Signed comparison */
    int64_t neg = -1LL;
    TEST(neg < 0);
    TEST(neg < a);
    TEST(0 > neg);
}

/* Test 64-bit bitwise operations */
void test_64bit_bitwise(void) {
    uint64_t a = 0xFFFFFFFF00000000ULL;
    uint64_t b = 0x00000000FFFFFFFFULL;
    
    TEST((a | b) == 0xFFFFFFFFFFFFFFFFULL);
    TEST((a & b) == 0x0000000000000000ULL);
    TEST((a ^ b) == 0xFFFFFFFFFFFFFFFFULL);
    TEST(~a == b);
    
    /* Shifts */
    uint64_t one = 1ULL;
    TEST((one << 32) == 0x100000000ULL);
    TEST((one << 63) == 0x8000000000000000ULL);
    
    uint64_t high = 0x8000000000000000ULL;
    TEST((high >> 32) == 0x80000000ULL);
    TEST((high >> 63) == 1ULL);
}

/* Test 64-bit multiplication */
void test_64bit_multiply(void) {
    int64_t a = 0x10000;  /* 64K */
    int64_t b = 0x10000;  /* 64K */
    int64_t product = a * b;
    TEST(product == 0x100000000LL);  /* 4GB */
    
    a = 1000000LL;
    b = 1000000LL;
    product = a * b;
    TEST(product == 1000000000000LL);  /* 1 trillion */
    
    /* Negative multiplication */
    a = -1000000LL;
    b = 1000LL;
    product = a * b;
    TEST(product == -1000000000LL);
    
    a = -1000LL;
    b = -1000LL;
    product = a * b;
    TEST(product == 1000000LL);
}

/* Test 64-bit division */
void test_64bit_divide(void) {
    int64_t a = 1000000000000LL;  /* 1 trillion */
    int64_t b = 1000000LL;  /* 1 million */
    int64_t quot = a / b;
    TEST(quot == 1000000LL);
    
    a = 0x100000000LL;  /* 4GB */
    b = 0x10000LL;  /* 64K */
    quot = a / b;
    TEST(quot == 0x10000LL);
    
    /* Negative division */
    a = -1000000000LL;
    b = 1000LL;
    quot = a / b;
    TEST(quot == -1000000LL);
    
    a = -1000000000LL;
    b = -1000LL;
    quot = a / b;
    TEST(quot == 1000000LL);
}

/* Test 64-bit modulo */
void test_64bit_modulo(void) {
    int64_t a = 1000000000001LL;
    int64_t b = 1000000LL;
    int64_t rem = a % b;
    TEST(rem == 1LL);
    
    a = 0x100000005LL;
    b = 0x10000LL;
    rem = a % b;
    TEST(rem == 5LL);
    
    /* Negative modulo */
    a = -1000000001LL;
    b = 1000000LL;
    rem = a % b;
    TEST(rem == -1LL);
}

/* Test 64-bit to 32-bit truncation */
void test_64bit_truncate(void) {
    int64_t a = 0x123456789ABCDEF0LL;
    int32_t low = (int32_t)a;
    TEST(low == (int32_t)0x9ABCDEF0);
    
    uint64_t b = 0xFFFFFFFFFFFFFFFFULL;
    uint32_t ulow = (uint32_t)b;
    TEST(ulow == 0xFFFFFFFFU);
}

/* Test 64-bit increment/decrement */
void test_64bit_incdec(void) {
    int64_t a = 0xFFFFFFFFLL;  /* 32-bit max */
    a++;
    TEST(a == 0x100000000LL);  /* should wrap to 64-bit */
    
    a = 0x100000000LL;
    a--;
    TEST(a == 0xFFFFFFFFLL);
}

int main(void) {
    test_64bit_basic();
    test_64bit_compare();
    test_64bit_bitwise();
    test_64bit_multiply();
    test_64bit_divide();
    test_64bit_modulo();
    test_64bit_truncate();
    test_64bit_incdec();
    
    return err;
}
