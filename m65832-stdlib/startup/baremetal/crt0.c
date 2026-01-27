/* crt0.c - M65832 Baremetal C Runtime Startup (C version)
 *
 * Minimal startup code that can be compiled with LLVM clang.
 * For full startup features (BSS init, constructors), use crt0.s
 * with the standalone assembler.
 */

#include <stddef.h>

/* Main function - provided by user program */
int main(void);

/* Linker-provided symbols (weak so we can compile without linker script) */
extern char __bss_start[] __attribute__((weak));
extern char __bss_end[] __attribute__((weak));

/* Initialize BSS to zero */
static void init_bss(void) {
    char *p = __bss_start;
    while (p < __bss_end) {
        *p++ = 0;
    }
}

/* Entry point */
__attribute__((section(".text.startup"), noreturn))
void _start(void) {
    /* Set up stack - done by reset vector or bootloader */
    
    /* Initialize BSS */
    init_bss();
    
    /* Call main */
    int ret = main();
    (void)ret;
    
    /* Halt processor */
    asm volatile("stp");
    __builtin_unreachable();
}

/* Platform hooks for stdlib */

__attribute__((noreturn))
void sys_exit(int status) {
    (void)status;
    asm volatile("stp");
    __builtin_unreachable();
}

__attribute__((noreturn))
void sys_abort(void) {
    asm volatile("stp");
    __builtin_unreachable();
}

/* Simple sbrk for malloc - bump allocator */
extern char __heap_start[] __attribute__((weak));
extern char __heap_end[] __attribute__((weak));

static char *heap_ptr = 0;

void *sys_sbrk(int incr) {
    if (heap_ptr == 0) {
        heap_ptr = __heap_start;
    }
    
    char *old_ptr = heap_ptr;
    char *new_ptr = heap_ptr + incr;
    
    if (new_ptr > __heap_end || new_ptr < __heap_start) {
        return (void *)-1;  /* Out of memory */
    }
    
    heap_ptr = new_ptr;
    return old_ptr;
}
