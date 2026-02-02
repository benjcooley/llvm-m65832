// Comprehensive pointer/array/function-pointer tests (no headers)

typedef unsigned int size_t;

#define NULL ((void *)0)

static int errors = 0;
#define CHECK(cond) do { if (!(cond)) errors++; } while (0)

static int add_ints(int a, int b) { return a + b; }
static int mul_ints(int a, int b) { return a * b; }

static int apply_binop(int (*fn)(int, int), int a, int b) {
  return fn(a, b);
}

static int sum_ints(const int *a, size_t n) {
  int sum = 0;
  size_t i = 0;
  while (i < n) {
    sum += a[i];
    i++;
  }
  return sum;
}

struct Node {
  int v;
  struct Node *next;
};

struct Ops {
  int (*mul)(int, int);
};

static int **make_pp(int *p) {
  static int *sp = 0;
  sp = p;
  return &sp;
}

static void test_basic_deref(void) {
  int x = 42;
  int *p = &x;
  CHECK(*p == 42);
  *p = 7;
  CHECK(x == 7);
}

static void test_pointer_arithmetic(void) {
  int arr[5] = {1, 2, 3, 4, 5};
  int *p = arr;
  CHECK(*(p + 2) == 3);
  p++;
  CHECK(*p == 2);
  CHECK(*(arr + 4) == 5);
  int *p2 = arr + 4;
  CHECK((p2 - arr) == 4);
  CHECK((arr + 1) < (arr + 3));
}

static void test_pointer_to_pointer(void) {
  int arr[4] = {10, 20, 30, 40};
  int *p = arr + 1;
  int **pp = &p;
  CHECK(**pp == 20);
  *pp = arr + 3;
  CHECK(**pp == 40);
}

static void test_pointer_as_array(void) {
  int arr[3] = {5, 6, 7};
  int *p = arr;
  CHECK(p[0] == 5);
  CHECK(p[2] == 7);
  p[1] = 9;
  CHECK(arr[1] == 9);
}

static void test_array_of_pointers(void) {
  char s1[3] = {'a', 'b', '\0'};
  char s2[3] = {'c', 'd', '\0'};
  char *arrp[2] = {s1, s2};
  CHECK(arrp[0][1] == 'b');
  CHECK(arrp[1][0] == 'c');
  CHECK(arrp[1][1] == 'd');
}

static void test_pointer_to_array(void) {
  int arr[5] = {1, 2, 3, 4, 5};
  int (*pa)[5] = &arr;
  CHECK((*pa)[3] == 4);
  (*pa)[0] = 10;
  CHECK(arr[0] == 10);
}

static void test_struct_pointer_arrow(void) {
  struct Node a;
  struct Node b;
  a.v = 1;
  b.v = 2;
  a.next = &b;
  CHECK(a.next->v == 2);
  a.next->v = 5;
  CHECK(b.v == 5);
}

static void test_function_pointers(void) {
  int (*fp)(int, int) = add_ints;
  CHECK(fp(2, 3) == 5);
  CHECK(apply_binop(mul_ints, 3, 4) == 12);
}

static void test_function_pointer_in_struct(void) {
  struct Ops ops;
  ops.mul = mul_ints;
  CHECK(ops.mul(4, 5) == 20);
}

static void test_pointer_to_function_pointer(void) {
  int (*fp)(int, int) = add_ints;
  int (**pp)(int, int) = &fp;
  CHECK((*pp)(1, 2) == 3);
}

static void test_make_pp(void) {
  int x = 11;
  int **pp = make_pp(&x);
  CHECK(**pp == 11);
  x = 13;
  CHECK(**pp == 13);
}

static void test_pointer_args(void) {
  int arr[4] = {1, 2, 3, 4};
  CHECK(sum_ints(arr, 4) == 10);
  CHECK(sum_ints(arr + 1, 2) == 5);
}

static void test_char_pointer_arithmetic(void) {
  unsigned char buf[4];
  unsigned char *p = buf;
  p[0] = 1;
  *(p + 1) = 2;
  *(p + 2) = 3;
  *(p + 3) = 4;
  CHECK(buf[0] == 1);
  CHECK(buf[3] == 4);
}

int main(void) {
  test_basic_deref();
  test_pointer_arithmetic();
  test_pointer_to_pointer();
  test_pointer_as_array();
  test_array_of_pointers();
  test_pointer_to_array();
  test_struct_pointer_arrow();
  test_function_pointers();
  test_function_pointer_in_struct();
  test_pointer_to_function_pointer();
  test_make_pp();
  test_pointer_args();
  test_char_pointer_arithmetic();
  return errors;
}
