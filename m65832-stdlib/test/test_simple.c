/* test_simple.c - Very simple sanity test */
#include <string.h>

int main(void) {
    /* Test strlen */
    if (strlen("Hello") != 5) return 1;
    if (strlen("") != 0) return 2;
    
    /* Test strcmp */
    if (strcmp("abc", "abc") != 0) return 3;
    if (strcmp("abc", "abd") >= 0) return 4;
    
    /* Test strcpy */
    char buf[16];
    strcpy(buf, "Test");
    if (strcmp(buf, "Test") != 0) return 5;
    
    /* All tests passed */
    return 0;
}
