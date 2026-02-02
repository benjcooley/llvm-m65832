/*
 * Basic integer operations test for M65832
 * No headers required - standalone test
 */

/* Test basic 32-bit arithmetic */
__attribute__((noinline))
int test_add(int a, int b) {
    return a + b;
}

__attribute__((noinline))
int test_sub(int a, int b) {
    return a - b;
}

__attribute__((noinline))
int test_mul(int a, int b) {
    return a * b;
}

__attribute__((noinline))
int test_div(int a, int b) {
    return a / b;
}

__attribute__((noinline))
int test_mod(int a, int b) {
    return a % b;
}

__attribute__((noinline))
unsigned test_udiv(unsigned a, unsigned b) {
    return a / b;
}

__attribute__((noinline))
unsigned test_umod(unsigned a, unsigned b) {
    return a % b;
}

/* Test shifts */
__attribute__((noinline))
int test_shl(int a, int b) {
    return a << b;
}

__attribute__((noinline))
int test_shr(int a, int b) {
    return a >> b;
}

__attribute__((noinline))
unsigned test_ushr(unsigned a, int b) {
    return a >> b;
}

/* Test bitwise ops */
__attribute__((noinline))
int test_and(int a, int b) {
    return a & b;
}

__attribute__((noinline))
int test_or(int a, int b) {
    return a | b;
}

__attribute__((noinline))
int test_xor(int a, int b) {
    return a ^ b;
}

__attribute__((noinline))
int test_not(int a) {
    return ~a;
}

/* Test comparisons */
__attribute__((noinline))
int test_eq(int a, int b) {
    return a == b;
}

__attribute__((noinline))
int test_ne(int a, int b) {
    return a != b;
}

__attribute__((noinline))
int test_lt(int a, int b) {
    return a < b;
}

__attribute__((noinline))
int test_le(int a, int b) {
    return a <= b;
}

__attribute__((noinline))
int test_gt(int a, int b) {
    return a > b;
}

__attribute__((noinline))
int test_ge(int a, int b) {
    return a >= b;
}

int main(void) {
    /* Basic arithmetic */
    if (test_add(10, 20) != 30) return 1;
    if (test_add(-5, 10) != 5) return 2;
    if (test_sub(20, 10) != 10) return 3;
    if (test_sub(10, 20) != -10) return 4;
    if (test_mul(6, 7) != 42) return 5;
    if (test_mul(-6, 7) != -42) return 6;
    if (test_div(42, 7) != 6) return 7;
    if (test_div(-42, 7) != -6) return 8;
    if (test_mod(17, 5) != 2) return 9;
    if (test_udiv(100, 10) != 10) return 10;
    if (test_umod(17, 5) != 2) return 11;
    
    /* Shifts */
    if (test_shl(1, 4) != 16) return 12;
    if (test_shl(0xFF, 8) != 0xFF00) return 13;
    if (test_shr(16, 2) != 4) return 14;
    if (test_shr(-16, 2) != -4) return 15;  /* Arithmetic shift */
    if (test_ushr(0x80000000, 4) != 0x08000000) return 16;
    
    /* Bitwise */
    if (test_and(0xFF, 0x0F) != 0x0F) return 17;
    if (test_or(0xF0, 0x0F) != 0xFF) return 18;
    if (test_xor(0xFF, 0x0F) != 0xF0) return 19;
    if (test_not(0) != -1) return 20;
    
    /* Comparisons */
    if (test_eq(5, 5) != 1) return 21;
    if (test_eq(5, 6) != 0) return 22;
    if (test_ne(5, 6) != 1) return 23;
    if (test_ne(5, 5) != 0) return 24;
    if (test_lt(3, 5) != 1) return 25;
    if (test_lt(5, 3) != 0) return 26;
    if (test_lt(-1, 1) != 1) return 27;
    if (test_le(3, 3) != 1) return 28;
    if (test_gt(5, 3) != 1) return 29;
    if (test_ge(3, 3) != 1) return 30;
    
    return 0;
}
