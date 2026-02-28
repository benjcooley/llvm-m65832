/*
 * Varargs printf with double - matches LLVM test-suite 2006-12-01-float_varg
 * Tests va_arg for f64 (double) in variadic calls.
 */
#include <stdio.h>

int main(void) {
    printf("foo %f %f %f %f\n", 1.23, 12312.1, 3.1, 13.1);
    return 0;
}
