/* test_math_basic.c - Basic math tests for M65832
 * Tests integer arithmetic and bit operations
 */

static int err = 0;

#define TEST(cond) do { if (!(cond)) err++; } while (0)

/* Test basic arithmetic */
void test_arithmetic(void) {
    /* Addition */
    TEST(1 + 1 == 2);
    TEST(100 + 200 == 300);
    TEST((-5) + 3 == -2);
    TEST((-5) + (-3) == -8);
    
    /* Subtraction */
    TEST(5 - 3 == 2);
    TEST(3 - 5 == -2);
    TEST((-5) - (-3) == -2);
    
    /* Multiplication */
    TEST(3 * 4 == 12);
    TEST((-3) * 4 == -12);
    TEST((-3) * (-4) == 12);
    TEST(100 * 100 == 10000);
    
    /* Division */
    TEST(12 / 3 == 4);
    TEST(12 / 5 == 2);
    TEST((-12) / 3 == -4);
    TEST((-12) / (-3) == 4);
    
    /* Modulo */
    TEST(12 % 5 == 2);
    TEST((-12) % 5 == -2);
    TEST(12 % (-5) == 2);
}

/* Test comparisons */
void test_comparisons(void) {
    TEST(1 < 2);
    TEST(2 > 1);
    TEST(1 <= 1);
    TEST(2 >= 2);
    TEST(1 <= 2);
    TEST(2 >= 1);
    TEST(1 == 1);
    TEST(1 != 2);
    
    /* Signed comparisons */
    TEST(-1 < 0);
    TEST(-1 < 1);
    TEST(-2 < -1);
    TEST(0 > -1);
}

/* Test bitwise operations */
void test_bitwise(void) {
    /* AND */
    TEST((0xFF & 0x0F) == 0x0F);
    TEST((0xAA & 0x55) == 0x00);
    TEST((0xFF & 0xFF) == 0xFF);
    
    /* OR */
    TEST((0xF0 | 0x0F) == 0xFF);
    TEST((0xAA | 0x55) == 0xFF);
    TEST((0x00 | 0x00) == 0x00);
    
    /* XOR */
    TEST((0xFF ^ 0x0F) == 0xF0);
    TEST((0xAA ^ 0x55) == 0xFF);
    TEST((0xFF ^ 0xFF) == 0x00);
    
    /* NOT */
    TEST((~0x00000000U) == 0xFFFFFFFFU);
    TEST((~0xFFFFFFFFU) == 0x00000000U);
    
    /* Shifts */
    TEST((1 << 0) == 1);
    TEST((1 << 4) == 16);
    TEST((1 << 8) == 256);
    TEST((16 >> 2) == 4);
    TEST((256 >> 4) == 16);
    
    /* Arithmetic right shift on signed */
    TEST(((-8) >> 1) == -4);
}

/* Test 32-bit boundaries */
void test_32bit(void) {
    unsigned int max = 0xFFFFFFFFU;
    int smax = 0x7FFFFFFF;
    int smin = (int)0x80000000;
    
    TEST(max == 4294967295U);
    TEST(smax == 2147483647);
    TEST(smin == -2147483648);
    
    /* Overflow behavior (unsigned wraps) */
    TEST(max + 1 == 0);
    TEST(0U - 1U == max);
    
    /* Large multiplications */
    TEST(0x10000U * 0x10000U == 0x00000000U);  /* Overflow */
    TEST(0x10000U * 0x1000U == 0x10000000U);
}

/* Test pointer arithmetic */
void test_pointers(void) {
    int arr[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int *p = arr;
    
    TEST(*p == 0);
    TEST(*(p + 1) == 1);
    TEST(*(p + 5) == 5);
    
    p = &arr[5];
    TEST(*p == 5);
    TEST(*(p - 2) == 3);
    
    /* Pointer difference */
    int *a = &arr[2];
    int *b = &arr[7];
    TEST(b - a == 5);
}

/* Test arrays and indexing */
void test_arrays(void) {
    int arr[5] = {10, 20, 30, 40, 50};
    
    TEST(arr[0] == 10);
    TEST(arr[2] == 30);
    TEST(arr[4] == 50);
    
    arr[2] = 100;
    TEST(arr[2] == 100);
    
    /* Multi-dimensional */
    int mat[2][3] = {{1, 2, 3}, {4, 5, 6}};
    TEST(mat[0][0] == 1);
    TEST(mat[0][2] == 3);
    TEST(mat[1][0] == 4);
    TEST(mat[1][2] == 6);
}

int main(void) {
    test_arithmetic();
    test_comparisons();
    test_bitwise();
    test_32bit();
    test_pointers();
    test_arrays();
    
    return err;
}
