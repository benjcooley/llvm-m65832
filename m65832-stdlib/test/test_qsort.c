/* test_qsort.c - Test qsort and bsearch */
#include <stdlib.h>
#include <string.h>

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name, condition) do { \
    if (condition) { tests_passed++; } \
    else { tests_failed++; } \
} while(0)

/* Compare functions */
static int int_cmp(const void *a, const void *b) {
    return *(const int *)a - *(const int *)b;
}

static int str_cmp(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

/* Test qsort with integers */
void test_qsort_int(void) {
    int arr[] = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    int n = sizeof(arr) / sizeof(arr[0]);
    
    qsort(arr, n, sizeof(int), int_cmp);
    
    /* Check sorted */
    int sorted = 1;
    for (int i = 1; i < n; i++) {
        if (arr[i] < arr[i-1]) sorted = 0;
    }
    TEST("qsort int sorted", sorted);
    TEST("qsort int first", arr[0] == 1);
    TEST("qsort int last", arr[n-1] == 9);
}

/* Test qsort with already sorted array */
void test_qsort_sorted(void) {
    int arr[] = {1, 2, 3, 4, 5};
    int n = sizeof(arr) / sizeof(arr[0]);
    
    qsort(arr, n, sizeof(int), int_cmp);
    
    TEST("qsort already sorted", arr[0] == 1 && arr[4] == 5);
}

/* Test qsort with reverse sorted array */
void test_qsort_reverse(void) {
    int arr[] = {5, 4, 3, 2, 1};
    int n = sizeof(arr) / sizeof(arr[0]);
    
    qsort(arr, n, sizeof(int), int_cmp);
    
    TEST("qsort reverse", arr[0] == 1 && arr[4] == 5);
}

/* Test qsort with duplicates */
void test_qsort_duplicates(void) {
    int arr[] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
    int n = sizeof(arr) / sizeof(arr[0]);
    
    qsort(arr, n, sizeof(int), int_cmp);
    
    int sorted = 1;
    for (int i = 1; i < n; i++) {
        if (arr[i] < arr[i-1]) sorted = 0;
    }
    TEST("qsort duplicates", sorted);
}

/* Test qsort with single element */
void test_qsort_single(void) {
    int arr[] = {42};
    qsort(arr, 1, sizeof(int), int_cmp);
    TEST("qsort single", arr[0] == 42);
}

/* Test qsort with empty array */
void test_qsort_empty(void) {
    int arr[1] = {0};  /* dummy */
    qsort(arr, 0, sizeof(int), int_cmp);  /* should not crash */
    TEST("qsort empty", 1);  /* if we get here, it passed */
}

/* Test qsort with strings */
void test_qsort_strings(void) {
    const char *arr[] = {"banana", "apple", "cherry", "date"};
    int n = sizeof(arr) / sizeof(arr[0]);
    
    qsort(arr, n, sizeof(char *), str_cmp);
    
    TEST("qsort str first", strcmp(arr[0], "apple") == 0);
    TEST("qsort str last", strcmp(arr[3], "date") == 0);
}

/* Test bsearch */
void test_bsearch(void) {
    int arr[] = {1, 3, 5, 7, 9, 11, 13, 15};
    int n = sizeof(arr) / sizeof(arr[0]);
    int key;
    int *found;
    
    /* Find existing element */
    key = 7;
    found = (int *)bsearch(&key, arr, n, sizeof(int), int_cmp);
    TEST("bsearch found", found != NULL && *found == 7);
    
    /* Find first element */
    key = 1;
    found = (int *)bsearch(&key, arr, n, sizeof(int), int_cmp);
    TEST("bsearch first", found != NULL && *found == 1);
    
    /* Find last element */
    key = 15;
    found = (int *)bsearch(&key, arr, n, sizeof(int), int_cmp);
    TEST("bsearch last", found != NULL && *found == 15);
    
    /* Search for missing element */
    key = 6;
    found = (int *)bsearch(&key, arr, n, sizeof(int), int_cmp);
    TEST("bsearch not found", found == NULL);
    
    /* Search for element below range */
    key = 0;
    found = (int *)bsearch(&key, arr, n, sizeof(int), int_cmp);
    TEST("bsearch below", found == NULL);
    
    /* Search for element above range */
    key = 100;
    found = (int *)bsearch(&key, arr, n, sizeof(int), int_cmp);
    TEST("bsearch above", found == NULL);
}

/* Test bsearch with strings */
void test_bsearch_strings(void) {
    const char *arr[] = {"apple", "banana", "cherry", "date", "elderberry"};
    int n = sizeof(arr) / sizeof(arr[0]);
    const char *key;
    const char **found;
    
    key = "cherry";
    found = (const char **)bsearch(&key, arr, n, sizeof(char *), str_cmp);
    TEST("bsearch str found", found != NULL && strcmp(*found, "cherry") == 0);
    
    key = "coconut";
    found = (const char **)bsearch(&key, arr, n, sizeof(char *), str_cmp);
    TEST("bsearch str not found", found == NULL);
}

int main(void) {
    test_qsort_int();
    test_qsort_sorted();
    test_qsort_reverse();
    test_qsort_duplicates();
    test_qsort_single();
    test_qsort_empty();
    test_qsort_strings();
    test_bsearch();
    test_bsearch_strings();
    
    /* Return 0 if all tests passed, otherwise number of failures */
    return tests_failed;
}
