/*
 * 64-bit integer operations test for M65832
 * No headers required - standalone test
 */

typedef unsigned long long uint64;
typedef long long int64;
typedef unsigned int uint32;

/* Basic 64-bit arithmetic */
__attribute__((noinline))
uint64 test_add64(uint64 a, uint64 b) {
    return a + b;
}

__attribute__((noinline))
uint64 test_sub64(uint64 a, uint64 b) {
    return a - b;
}

__attribute__((noinline))
uint64 test_mul64(uint64 a, uint64 b) {
    return a * b;
}

__attribute__((noinline))
uint64 test_div64(uint64 a, uint64 b) {
    return a / b;
}

__attribute__((noinline))
uint64 test_mod64(uint64 a, uint64 b) {
    return a % b;
}

/* 64-bit shifts */
__attribute__((noinline))
uint64 test_shl64(uint64 a, int b) {
    return a << b;
}

__attribute__((noinline))
uint64 test_shr64(uint64 a, int b) {
    return a >> b;
}

__attribute__((noinline))
int64 test_sar64(int64 a, int b) {
    return a >> b;
}

/* Extract low and high words */
__attribute__((noinline))
uint32 get_low(uint64 v) {
    return (uint32)v;
}

__attribute__((noinline))
uint32 get_high(uint64 v) {
    return (uint32)(v >> 32);
}

/* Combine words */
__attribute__((noinline))
uint64 make64(uint32 hi, uint32 lo) {
    return ((uint64)hi << 32) | lo;
}

/* 64-bit comparisons */
__attribute__((noinline))
int test_eq64(uint64 a, uint64 b) {
    return a == b;
}

__attribute__((noinline))
int test_lt64(uint64 a, uint64 b) {
    return a < b;
}

__attribute__((noinline))
int test_gt64(uint64 a, uint64 b) {
    return a > b;
}

__attribute__((optnone))
int main(void) {
    uint64 a, b, r;
    
    /* Test make64/get_low/get_high */
    a = make64(0x12345678, 0xDEADBEEF);
    if (get_low(a) != 0xDEADBEEF) return 1;
    if (get_high(a) != 0x12345678) return 2;
    
    /* Test 64-bit add */
    a = make64(0, 0xFFFFFFFF);
    b = make64(0, 1);
    r = test_add64(a, b);
    if (get_low(r) != 0) return 3;
    if (get_high(r) != 1) return 4;
    
    /* Test 64-bit sub */
    a = make64(1, 0);
    b = make64(0, 1);
    r = test_sub64(a, b);
    if (get_low(r) != 0xFFFFFFFF) return 5;
    if (get_high(r) != 0) return 6;
    
    /* Test 64-bit mul (small values) */
    a = 1000;
    b = 1000;
    r = test_mul64(a, b);
    if (r != 1000000) return 7;
    
    /* Test 64-bit mul (larger) */
    a = 0x10000;
    b = 0x10000;
    r = test_mul64(a, b);
    if (get_low(r) != 0) return 8;
    if (get_high(r) != 1) return 9;
    
    /* Test 64-bit div */
    a = 1000000;
    b = 1000;
    r = test_div64(a, b);
    if (r != 1000) return 10;
    
    /* Test 64-bit mod */
    a = 1000007;
    b = 1000;
    r = test_mod64(a, b);
    if (r != 7) return 11;
    
    /* Test 64-bit shifts */
    a = 1;
    r = test_shl64(a, 32);
    if (get_low(r) != 0) return 12;
    if (get_high(r) != 1) return 13;
    
    a = make64(1, 0);
    r = test_shr64(a, 32);
    if (r != 1) return 14;
    
    a = make64(0x80000000, 0);
    r = test_shr64(a, 16);
    if (get_high(r) != 0x8000) return 15;
    if (get_low(r) != 0) return 16;
    
    /* Test 64-bit comparisons */
    a = make64(1, 0);
    b = make64(0, 0xFFFFFFFF);
    if (test_lt64(a, b) != 0) return 17;  /* a > b */
    if (test_gt64(a, b) != 1) return 18;
    
    a = make64(0, 100);
    b = make64(0, 100);
    if (test_eq64(a, b) != 1) return 19;
    
    return 0;
}
