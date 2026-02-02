/* test_control_flow.c - Test control flow constructs
 * Validates if/else, switch, loops, goto, break, continue
 */

static int err = 0;

#define TEST(cond) do { if (!(cond)) err++; } while (0)

/* If/else tests */
int test_if(int x) {
    if (x < 0) return -1;
    else if (x == 0) return 0;
    else return 1;
}

/* Nested if */
int test_nested_if(int x, int y) {
    if (x > 0) {
        if (y > 0) return 1;
        else return 2;
    } else {
        if (y > 0) return 3;
        else return 4;
    }
}

/* Switch statement */
int test_switch(int x) {
    switch (x) {
        case 0: return 100;
        case 1: return 101;
        case 2: return 102;
        case 5: return 105;
        case 10: return 110;
        default: return -1;
    }
}

/* Switch with fallthrough */
int test_switch_fallthrough(int x) {
    int result = 0;
    switch (x) {
        case 3:
            result += 100;
            /* fallthrough */
        case 2:
            result += 10;
            /* fallthrough */
        case 1:
            result += 1;
            break;
        default:
            result = -1;
    }
    return result;
}

/* While loop */
int test_while(int n) {
    int sum = 0;
    int i = 1;
    while (i <= n) {
        sum += i;
        i++;
    }
    return sum;  /* 1+2+...+n = n*(n+1)/2 */
}

/* Do-while loop */
int test_do_while(int n) {
    int sum = 0;
    int i = 1;
    do {
        sum += i;
        i++;
    } while (i <= n);
    return sum;
}

/* For loop */
int test_for(int n) {
    int sum = 0;
    for (int i = 1; i <= n; i++) {
        sum += i;
    }
    return sum;
}

/* Nested loops */
int test_nested_loops(int rows, int cols) {
    int sum = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            sum += i * cols + j;
        }
    }
    return sum;  /* sum of 0..(rows*cols-1) */
}

/* Break in loop */
int test_break(int n) {
    int sum = 0;
    for (int i = 1; i <= 100; i++) {
        if (i > n) break;
        sum += i;
    }
    return sum;
}

/* Continue in loop */
int test_continue(int n) {
    int sum = 0;
    for (int i = 1; i <= n; i++) {
        if (i % 2 == 0) continue;  /* skip even numbers */
        sum += i;
    }
    return sum;  /* sum of odd numbers 1..n */
}

/* Goto (basic use) */
int test_goto(int x) {
    if (x < 0) goto negative;
    if (x == 0) goto zero;
    return 1;
negative:
    return -1;
zero:
    return 0;
}

/* Ternary operator */
int test_ternary(int x) {
    return x > 0 ? 1 : (x < 0 ? -1 : 0);
}

/* Short-circuit evaluation */
int global_counter = 0;

int increment_and_return(int v) {
    global_counter++;
    return v;
}

int test_short_circuit(void) {
    global_counter = 0;
    
    /* && should short-circuit on false */
    if (0 && increment_and_return(1)) { }
    if (global_counter != 0) return 1;
    
    /* || should short-circuit on true */
    if (1 || increment_and_return(1)) { }
    if (global_counter != 0) return 2;
    
    /* && should evaluate second on true */
    if (1 && increment_and_return(1)) { }
    if (global_counter != 1) return 3;
    
    global_counter = 0;
    /* || should evaluate second on false */
    if (0 || increment_and_return(1)) { }
    if (global_counter != 1) return 4;
    
    return 0;  /* success */
}

void run_tests(void) {
    /* If tests */
    TEST(test_if(-5) == -1);
    TEST(test_if(0) == 0);
    TEST(test_if(5) == 1);
    
    /* Nested if tests */
    TEST(test_nested_if(1, 1) == 1);
    TEST(test_nested_if(1, -1) == 2);
    TEST(test_nested_if(-1, 1) == 3);
    TEST(test_nested_if(-1, -1) == 4);
    
    /* Switch tests */
    TEST(test_switch(0) == 100);
    TEST(test_switch(1) == 101);
    TEST(test_switch(5) == 105);
    TEST(test_switch(10) == 110);
    TEST(test_switch(99) == -1);
    
    /* Switch fallthrough */
    TEST(test_switch_fallthrough(3) == 111);
    TEST(test_switch_fallthrough(2) == 11);
    TEST(test_switch_fallthrough(1) == 1);
    TEST(test_switch_fallthrough(0) == -1);
    
    /* While loop */
    TEST(test_while(10) == 55);  /* 1+2+...+10 */
    TEST(test_while(0) == 0);
    
    /* Do-while loop */
    TEST(test_do_while(10) == 55);
    TEST(test_do_while(1) == 1);
    
    /* For loop */
    TEST(test_for(10) == 55);
    TEST(test_for(100) == 5050);
    
    /* Nested loops */
    TEST(test_nested_loops(3, 4) == 66);  /* 0+1+2+...+11 */
    
    /* Break */
    TEST(test_break(5) == 15);  /* 1+2+3+4+5 */
    TEST(test_break(10) == 55);
    
    /* Continue */
    TEST(test_continue(10) == 25);  /* 1+3+5+7+9 */
    TEST(test_continue(9) == 25);
    
    /* Goto */
    TEST(test_goto(-1) == -1);
    TEST(test_goto(0) == 0);
    TEST(test_goto(1) == 1);
    
    /* Ternary */
    TEST(test_ternary(-5) == -1);
    TEST(test_ternary(0) == 0);
    TEST(test_ternary(5) == 1);
    
    /* Short-circuit */
    TEST(test_short_circuit() == 0);
}

int main(void) {
    run_tests();
    return err;
}
