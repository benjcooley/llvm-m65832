/* test_ctype_basic.c - Basic ctype function tests */
#include <ctype.h>

static int test_count = 0;
static int fail_count = 0;

#define TEST(cond) do { test_count++; if (!(cond)) fail_count++; } while(0)

int main(void) {
    /* isalpha */
    TEST(isalpha('A') != 0);
    TEST(isalpha('z') != 0);
    TEST(isalpha('5') == 0);
    TEST(isalpha(' ') == 0);
    
    /* isdigit */
    TEST(isdigit('0') != 0);
    TEST(isdigit('9') != 0);
    TEST(isdigit('a') == 0);
    
    /* isalnum */
    TEST(isalnum('A') != 0);
    TEST(isalnum('5') != 0);
    TEST(isalnum(' ') == 0);
    
    /* islower/isupper */
    TEST(islower('a') != 0);
    TEST(islower('A') == 0);
    TEST(isupper('A') != 0);
    TEST(isupper('a') == 0);
    
    /* isspace */
    TEST(isspace(' ') != 0);
    TEST(isspace('\t') != 0);
    TEST(isspace('\n') != 0);
    TEST(isspace('a') == 0);
    
    /* tolower/toupper */
    TEST(tolower('A') == 'a');
    TEST(tolower('Z') == 'z');
    TEST(tolower('a') == 'a');
    TEST(toupper('a') == 'A');
    TEST(toupper('z') == 'Z');
    TEST(toupper('A') == 'A');
    
    /* isxdigit */
    TEST(isxdigit('0') != 0);
    TEST(isxdigit('f') != 0);
    TEST(isxdigit('F') != 0);
    TEST(isxdigit('g') == 0);
    
    /* isprint/isgraph/ispunct */
    TEST(isprint(' ') != 0);
    TEST(isprint('a') != 0);
    TEST(isprint('\t') == 0);
    TEST(isgraph('a') != 0);
    TEST(isgraph(' ') == 0);
    TEST(ispunct('!') != 0);
    TEST(ispunct('a') == 0);
    
    /* iscntrl */
    TEST(iscntrl('\0') != 0);
    TEST(iscntrl('\n') != 0);
    TEST(iscntrl('a') == 0);
    
    return fail_count;
}
