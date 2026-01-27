/* test_ctype_step.c - Step by step ctype test */
#include <ctype.h>

int main(void) {
    int fail = 0;
    
    /* Test 1: isalpha('A') */
    if (isalpha('A') == 0) fail++;
    
    /* Test 2: isalpha('z') */
    if (isalpha('z') == 0) fail++;
    
    /* Test 3: isalpha('5') should be 0 */
    if (isalpha('5') != 0) fail++;
    
    /* Test 4: isdigit('5') */
    if (isdigit('5') == 0) fail++;
    
    /* Test 5: isdigit('A') should be 0 */
    if (isdigit('A') != 0) fail++;
    
    /* Test 6: tolower('A') */
    if (tolower('A') != 'a') fail++;
    
    /* Test 7: toupper('a') */
    if (toupper('a') != 'A') fail++;
    
    return fail;
}
