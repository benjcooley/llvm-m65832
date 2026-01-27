/* test_ctype.c - Comprehensive ctype.h tests */
#include <ctype.h>

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name, condition) do { \
    if (condition) { tests_passed++; } \
    else { tests_failed++; } \
} while(0)

/* Test isalpha */
void test_isalpha(void) {
    TEST("isalpha A", isalpha('A') != 0);
    TEST("isalpha Z", isalpha('Z') != 0);
    TEST("isalpha a", isalpha('a') != 0);
    TEST("isalpha z", isalpha('z') != 0);
    TEST("isalpha 0", isalpha('0') == 0);
    TEST("isalpha space", isalpha(' ') == 0);
    TEST("isalpha punct", isalpha('!') == 0);
}

/* Test isdigit */
void test_isdigit(void) {
    TEST("isdigit 0", isdigit('0') != 0);
    TEST("isdigit 9", isdigit('9') != 0);
    TEST("isdigit 5", isdigit('5') != 0);
    TEST("isdigit a", isdigit('a') == 0);
    TEST("isdigit A", isdigit('A') == 0);
    TEST("isdigit space", isdigit(' ') == 0);
}

/* Test isxdigit */
void test_isxdigit(void) {
    TEST("isxdigit 0", isxdigit('0') != 0);
    TEST("isxdigit 9", isxdigit('9') != 0);
    TEST("isxdigit a", isxdigit('a') != 0);
    TEST("isxdigit f", isxdigit('f') != 0);
    TEST("isxdigit A", isxdigit('A') != 0);
    TEST("isxdigit F", isxdigit('F') != 0);
    TEST("isxdigit g", isxdigit('g') == 0);
    TEST("isxdigit G", isxdigit('G') == 0);
}

/* Test isalnum */
void test_isalnum(void) {
    TEST("isalnum A", isalnum('A') != 0);
    TEST("isalnum z", isalnum('z') != 0);
    TEST("isalnum 5", isalnum('5') != 0);
    TEST("isalnum space", isalnum(' ') == 0);
    TEST("isalnum punct", isalnum('!') == 0);
}

/* Test islower */
void test_islower(void) {
    TEST("islower a", islower('a') != 0);
    TEST("islower z", islower('z') != 0);
    TEST("islower A", islower('A') == 0);
    TEST("islower 0", islower('0') == 0);
}

/* Test isupper */
void test_isupper(void) {
    TEST("isupper A", isupper('A') != 0);
    TEST("isupper Z", isupper('Z') != 0);
    TEST("isupper a", isupper('a') == 0);
    TEST("isupper 0", isupper('0') == 0);
}

/* Test isspace */
void test_isspace(void) {
    TEST("isspace space", isspace(' ') != 0);
    TEST("isspace tab", isspace('\t') != 0);
    TEST("isspace newline", isspace('\n') != 0);
    TEST("isspace return", isspace('\r') != 0);
    TEST("isspace formfeed", isspace('\f') != 0);
    TEST("isspace vtab", isspace('\v') != 0);
    TEST("isspace a", isspace('a') == 0);
    TEST("isspace 0", isspace('0') == 0);
}

/* Test isprint */
void test_isprint(void) {
    TEST("isprint space", isprint(' ') != 0);
    TEST("isprint a", isprint('a') != 0);
    TEST("isprint ~", isprint('~') != 0);
    TEST("isprint tab", isprint('\t') == 0);
    TEST("isprint null", isprint('\0') == 0);
    TEST("isprint del", isprint(127) == 0);
}

/* Test isgraph */
void test_isgraph(void) {
    TEST("isgraph a", isgraph('a') != 0);
    TEST("isgraph !", isgraph('!') != 0);
    TEST("isgraph space", isgraph(' ') == 0);
    TEST("isgraph tab", isgraph('\t') == 0);
}

/* Test ispunct */
void test_ispunct(void) {
    TEST("ispunct !", ispunct('!') != 0);
    TEST("ispunct .", ispunct('.') != 0);
    TEST("ispunct @", ispunct('@') != 0);
    TEST("ispunct a", ispunct('a') == 0);
    TEST("ispunct 0", ispunct('0') == 0);
    TEST("ispunct space", ispunct(' ') == 0);
}

/* Test iscntrl */
void test_iscntrl(void) {
    TEST("iscntrl null", iscntrl('\0') != 0);
    TEST("iscntrl tab", iscntrl('\t') != 0);
    TEST("iscntrl newline", iscntrl('\n') != 0);
    TEST("iscntrl del", iscntrl(127) != 0);
    TEST("iscntrl space", iscntrl(' ') == 0);
    TEST("iscntrl a", iscntrl('a') == 0);
}

/* Test tolower */
void test_tolower(void) {
    TEST("tolower A", tolower('A') == 'a');
    TEST("tolower Z", tolower('Z') == 'z');
    TEST("tolower a", tolower('a') == 'a');
    TEST("tolower 0", tolower('0') == '0');
    TEST("tolower !", tolower('!') == '!');
}

/* Test toupper */
void test_toupper(void) {
    TEST("toupper a", toupper('a') == 'A');
    TEST("toupper z", toupper('z') == 'Z');
    TEST("toupper A", toupper('A') == 'A');
    TEST("toupper 0", toupper('0') == '0');
    TEST("toupper !", toupper('!') == '!');
}

int main(void) {
    test_isalpha();
    test_isdigit();
    test_isxdigit();
    test_isalnum();
    test_islower();
    test_isupper();
    test_isspace();
    test_isprint();
    test_isgraph();
    test_ispunct();
    test_iscntrl();
    test_tolower();
    test_toupper();
    
    /* Return 0 if all tests passed, otherwise number of failures */
    return tests_failed;
}
