/* test_functions.c - Test function calls, stack, and control flow
 * Validates compiler codegen for functions with various signatures
 */

static int err = 0;

#define TEST(cond) do { if (!(cond)) err++; } while (0)

/* Simple function - no args, no return */
static int call_count = 0;
void increment_counter(void) {
    call_count++;
}

/* Function with return value */
int return_42(void) {
    return 42;
}

/* Function with one argument */
int double_it(int x) {
    return x * 2;
}

/* Function with multiple arguments */
int add_three(int a, int b, int c) {
    return a + b + c;
}

/* Function with many arguments (tests stack passing) */
int sum_eight(int a, int b, int c, int d, int e, int f, int g, int h) {
    return a + b + c + d + e + f + g + h;
}

/* Recursive function */
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

/* Fibonacci - tests multiple recursive calls */
int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

/* Local variables test */
int locals_test(int x) {
    int a = x + 1;
    int b = a + 1;
    int c = b + 1;
    int d = c + 1;
    int e = d + 1;
    return a + b + c + d + e;  /* x+1 + x+2 + x+3 + x+4 + x+5 = 5x + 15 */
}

/* Array on stack */
int array_stack_test(void) {
    int arr[5];
    for (int i = 0; i < 5; i++) {
        arr[i] = i * 10;
    }
    return arr[0] + arr[1] + arr[2] + arr[3] + arr[4];  /* 0+10+20+30+40 = 100 */
}

/* Nested function calls */
int level3(int x) { return x + 1; }
int level2(int x) { return level3(x) + 1; }
int level1(int x) { return level2(x) + 1; }
int nested_calls(int x) {
    return level1(x);  /* x + 3 */
}

/* Pointer argument */
void swap(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

/* Struct return (by value if small) */
struct point { int x; int y; };

struct point make_point(int x, int y) {
    struct point p;
    p.x = x;
    p.y = y;
    return p;
}

/* Struct argument */
int point_sum(struct point p) {
    return p.x + p.y;
}

/* Test all functions */
void test_basic_calls(void) {
    call_count = 0;
    increment_counter();
    increment_counter();
    increment_counter();
    TEST(call_count == 3);
    
    TEST(return_42() == 42);
    TEST(double_it(21) == 42);
    TEST(add_three(1, 2, 3) == 6);
}

void test_many_args(void) {
    TEST(sum_eight(1, 2, 3, 4, 5, 6, 7, 8) == 36);
    TEST(sum_eight(10, 20, 30, 40, 50, 60, 70, 80) == 360);
}

void test_recursion(void) {
    TEST(factorial(0) == 1);
    TEST(factorial(1) == 1);
    TEST(factorial(5) == 120);
    TEST(factorial(6) == 720);
    
    TEST(fib(0) == 0);
    TEST(fib(1) == 1);
    TEST(fib(5) == 5);
    TEST(fib(10) == 55);
}

void test_locals(void) {
    TEST(locals_test(0) == 15);
    TEST(locals_test(10) == 65);  /* 5*10 + 15 */
    TEST(array_stack_test() == 100);
}

void test_nested(void) {
    TEST(nested_calls(0) == 3);
    TEST(nested_calls(10) == 13);
}

void test_pointers(void) {
    int a = 10, b = 20;
    swap(&a, &b);
    TEST(a == 20 && b == 10);
}

void test_structs(void) {
    struct point p = make_point(3, 4);
    TEST(p.x == 3 && p.y == 4);
    TEST(point_sum(p) == 7);
}

int main(void) {
    test_basic_calls();
    test_many_args();
    test_recursion();
    test_locals();
    test_nested();
    test_pointers();
    test_structs();
    
    return err;
}
