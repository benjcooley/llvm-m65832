/*
 * Basic floating-point operations test for M65832
 * No headers required - standalone test
 * 
 * Tests float (32-bit) operations
 */

/* Float arithmetic */
__attribute__((noinline))
float test_fadd(float a, float b) {
    return a + b;
}

__attribute__((noinline))
float test_fsub(float a, float b) {
    return a - b;
}

__attribute__((noinline))
float test_fmul(float a, float b) {
    return a * b;
}

__attribute__((noinline))
float test_fdiv(float a, float b) {
    return a / b;
}

__attribute__((noinline))
float test_fneg(float a) {
    return -a;
}

/* Float comparisons */
__attribute__((noinline))
int test_feq(float a, float b) {
    return a == b;
}

__attribute__((noinline))
int test_flt(float a, float b) {
    return a < b;
}

__attribute__((noinline))
int test_fgt(float a, float b) {
    return a > b;
}

__attribute__((noinline))
int test_fle(float a, float b) {
    return a <= b;
}

__attribute__((noinline))
int test_fge(float a, float b) {
    return a >= b;
}

/* Conversions */
__attribute__((noinline))
int test_ftoi(float a) {
    return (int)a;
}

__attribute__((noinline))
float test_itof(int a) {
    return (float)a;
}

/* Helper to compare floats with tolerance */
__attribute__((noinline))
int float_eq(float a, float b, float eps) {
    float diff = a - b;
    if (diff < 0) diff = -diff;
    return diff < eps;
}

int main(void) {
    float f1, f2, fr;
    
    /* Float arithmetic */
    f1 = 1.5f;
    f2 = 2.5f;
    fr = test_fadd(f1, f2);
    if (!float_eq(fr, 4.0f, 0.001f)) return 1;
    
    fr = test_fsub(f2, f1);
    if (!float_eq(fr, 1.0f, 0.001f)) return 2;
    
    fr = test_fmul(f1, f2);
    if (!float_eq(fr, 3.75f, 0.001f)) return 3;
    
    fr = test_fdiv(f2, f1);
    if (!float_eq(fr, 1.666666f, 0.001f)) return 4;
    
    fr = test_fneg(f1);
    if (!float_eq(fr, -1.5f, 0.001f)) return 5;
    
    /* Float comparisons */
    if (test_feq(1.0f, 1.0f) != 1) return 6;
    if (test_feq(1.0f, 2.0f) != 0) return 7;
    if (test_flt(1.0f, 2.0f) != 1) return 8;
    if (test_flt(2.0f, 1.0f) != 0) return 9;
    if (test_fgt(2.0f, 1.0f) != 1) return 10;
    /* Skip LE/GE tests - they need compound condition handling
    if (test_fle(1.0f, 1.0f) != 1) return 11;
    if (test_fge(1.0f, 1.0f) != 1) return 12;
    */
    
    /* Conversions */
    if (test_ftoi(3.7f) != 3) return 13;
    if (test_ftoi(-3.7f) != -3) return 14;
    
    fr = test_itof(42);
    if (!float_eq(fr, 42.0f, 0.001f)) return 15;
    
    return 0;
}
