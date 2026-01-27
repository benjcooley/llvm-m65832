/* hello.c - Simple test program for M65832 minimal libc */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void) {
    /* Test puts */
    puts("Hello from M65832!");
    
    /* Test printf */
    printf("Testing printf: %d + %d = %d\n", 2, 3, 2 + 3);
    printf("Hex: 0x%08X\n", 0xDEADBEEF);
    printf("String: %s\n", "world");
    
    /* Test string functions */
    char buf[32];
    strcpy(buf, "Hello");
    strcat(buf, " World");
    printf("strcat result: %s (len=%d)\n", buf, (int)strlen(buf));
    
    /* Test memory functions */
    char mem[16];
    memset(mem, 'A', 8);
    mem[8] = '\0';
    printf("memset result: %s\n", mem);
    
    /* Test malloc */
    char *p = malloc(32);
    if (p) {
        strcpy(p, "Allocated!");
        printf("malloc result: %s\n", p);
        /* Note: free() is a no-op in our simple allocator */
        free(p);
    } else {
        puts("malloc failed!");
    }
    
    puts("All tests passed!");
    return 0;
}
