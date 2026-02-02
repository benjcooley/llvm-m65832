/*
 * Basic double-precision floating-point operations test for M65832
 * No headers required - standalone test
 */

/* Double arithmetic */
__attribute__((noinline))
double test_dadd(double a, double b) {
    return a + b;
}

__attribute__((noinline))
double test_dsub(double a, double b) {
    return a - b;
}

__attribute__((noinline))
double test_dmul(double a, double b) {
    return a * b;
}

__attribute__((noinline))
double test_ddiv(double a, double b) {
    return a / b;
}

__attribute__((noinline))
double test_dneg(double a) {
    return -a;
}

/* Double comparisons */
__attribute__((noinline))
int test_deq(double a, double b) {
    return a == b;
}

__attribute__((noinline))
int test_dlt(double a, double b) {
    return a < b;
}

__attribute__((noinline))
int test_dgt(double a, double b) {
    return a > b;
}

/* Conversions */
__attribute__((noinline))
int test_dtoi(double a) {
    return (int)a;
}

__attribute__((noinline))
double test_itod(int a) {
    return (double)a;
}

/* Helper to compare doubles with tolerance */
__attribute__((noinline))
int double_eq(double a, double b, double eps) {
    double diff = a - b;
    if (diff < 0) diff = -diff;
    return diff < eps;
}

int main(void) {
    double d1, d2, dr;
    
    /* Double arithmetic */
    d1 = 1.5;
    d2 = 2.5;
    dr = test_dadd(d1, d2);
    if (!double_eq(dr, 4.0, 0.0001)) return 1;
    
    dr = test_dsub(d2, d1);
    if (!double_eq(dr, 1.0, 0.0001)) return 2;
    
    dr = test_dmul(d1, d2);
    if (!double_eq(dr, 3.75, 0.0001)) return 3;
    
    dr = test_ddiv(d2, d1);
    if (!double_eq(dr, 1.666666, 0.0001)) return 4;
    
    dr = test_dneg(d1);
    if (!double_eq(dr, -1.5, 0.0001)) return 5;
    
    /* Double comparisons */
    if (test_deq(1.0, 1.0) != 1) return 6;
    if (test_deq(1.0, 2.0) != 0) return 7;
    if (test_dlt(1.0, 2.0) != 1) return 8;
    if (test_dgt(2.0, 1.0) != 1) return 9;
    
    /* Conversions */
    if (test_dtoi(3.7) != 3) return 10;
    
    dr = test_itod(42);
    if (!double_eq(dr, 42.0, 0.0001)) return 11;
    
    return 0;
}
