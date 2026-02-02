/*
 * Standalone varargs test for M65832
 * Tests the varargs ABI without depending on stdarg.h
 * 
 * M65832 Calling Convention:
 *   - R0-R7 hold first 8 integer arguments
 *   - Additional arguments go on stack
 *   - For varargs: callee saves register args to stack, va_list points there
 */

/* Use compiler-provided va_list without headers */
typedef __builtin_va_list va_list_t;

/* Manually implement va_start/va_arg based on the ABI */
#define VA_START(ap, last) \
    __builtin_va_start(ap, last)

#define VA_ARG(ap, type) \
    __builtin_va_arg(ap, type)

#define VA_END(ap) \
    __builtin_va_end(ap)

/* Test 1: Sum integers (uses register args only) */
__attribute__((noinline))
int sum_ints_3(int count, ...) {
    va_list_t ap;
    VA_START(ap, count);
    int total = 0;
    for (int i = 0; i < count; i++) {
        total += VA_ARG(ap, int);
    }
    VA_END(ap);
    return total;
}

/* Test 2: Sum with specific known values */
__attribute__((noinline))
int sum_two(int first, ...) {
    va_list_t ap;
    VA_START(ap, first);
    int second = VA_ARG(ap, int);
    VA_END(ap);
    return first + second;
}

/* Test 3: Multiple types in sequence */
__attribute__((noinline))
int mixed_args(int a, ...) {
    va_list_t ap;
    VA_START(ap, a);
    int b = VA_ARG(ap, int);
    int c = VA_ARG(ap, int);
    int d = VA_ARG(ap, int);
    VA_END(ap);
    return a + b + c + d;
}

/* Test 4: More args (7 varargs = all in registers) */
__attribute__((noinline))
int sum_7_varargs(int count, ...) {
    va_list_t ap;
    VA_START(ap, count);
    int total = 0;
    for (int i = 0; i < count; i++) {
        total += VA_ARG(ap, int);
    }
    VA_END(ap);
    return total;
}

/* Test 5: 8 varargs (one spills to stack) */
__attribute__((noinline))
int sum_8_varargs(int count, ...) {
    va_list_t ap;
    VA_START(ap, count);
    int total = 0;
    for (int i = 0; i < count; i++) {
        total += VA_ARG(ap, int);
    }
    VA_END(ap);
    return total;
}

/* Test 6: Passing local variables to varargs */
__attribute__((noinline))
int test_local_vars(void) {
    int x = 10;
    int y = 20;
    int z = 30;
    return sum_ints_3(3, x, y, z);
}

int main(void) {
    int result;
    
    /* Test 1: Basic sum of 3 */
    result = sum_ints_3(3, 1, 2, 3);
    if (result != 6) return 1;
    
    /* Test 2: Sum two specific values */
    result = sum_two(42, 58);
    if (result != 100) return 2;
    
    /* Test 3: Mixed args */
    result = mixed_args(1, 2, 3, 4);
    if (result != 10) return 3;
    
    /* Test 4: 7 varargs (all fit in R1-R7) */
    result = sum_7_varargs(7, 1, 2, 3, 4, 5, 6, 7);
    if (result != 28) return 4;
    
    /* Test 5: 8 varargs (8th spills to stack) */
    result = sum_8_varargs(8, 1, 2, 3, 4, 5, 6, 7, 8);
    if (result != 36) return 5;
    
    /* Test 6: Local variables */
    result = test_local_vars();
    if (result != 60) return 6;
    
    return 0;
}
