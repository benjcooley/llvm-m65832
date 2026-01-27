/* test_string_basic.c - Basic string function tests */
#include <string.h>

static int test_count = 0;
static int fail_count = 0;

#define TEST(cond) do { test_count++; if (!(cond)) fail_count++; } while(0)

int main(void) {
    char buf[64];
    
    /* strlen tests */
    TEST(strlen("") == 0);
    TEST(strlen("x") == 1);
    TEST(strlen("Hello") == 5);
    TEST(strlen("Hello World") == 11);
    
    /* strcpy tests */
    strcpy(buf, "Hello");
    TEST(buf[0] == 'H');
    TEST(buf[4] == 'o');
    TEST(buf[5] == '\0');
    
    /* strcmp tests */
    TEST(strcmp("abc", "abc") == 0);
    TEST(strcmp("abc", "abd") < 0);
    TEST(strcmp("abd", "abc") > 0);
    TEST(strcmp("ab", "abc") < 0);
    
    /* strcat tests */
    strcpy(buf, "Hello");
    strcat(buf, " World");
    TEST(strcmp(buf, "Hello World") == 0);
    
    /* memcpy tests */
    memset(buf, 0, sizeof(buf));
    memcpy(buf, "Test", 4);
    TEST(buf[0] == 'T');
    TEST(buf[3] == 't');
    
    /* memset tests */
    memset(buf, 'A', 5);
    TEST(buf[0] == 'A');
    TEST(buf[4] == 'A');
    
    /* memcmp tests */
    TEST(memcmp("abc", "abc", 3) == 0);
    TEST(memcmp("abc", "abd", 3) < 0);
    
    return fail_count;
}
