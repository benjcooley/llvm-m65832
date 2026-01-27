/* test_ctype_min.c - Minimal ctype test */
#include <ctype.h>

int main(void) {
    /* Just test one function */
    if (isalpha('A') == 0) return 1;
    if (isalpha('5') != 0) return 2;
    return 0;
}
