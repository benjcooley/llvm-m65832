/* crt0.c - C Runtime startup for M65832 with Picolibc
 *
 * This is the entry point called by the reset vector.
 * It initializes the C runtime environment and calls main().
 */

#include <stdint.h>

/* Linker-provided symbols */
extern uint32_t _bss_start[];
extern uint32_t _bss_end[];
extern uint32_t _data_start[];
extern uint32_t _data_end[];
extern uint32_t _data_load[];
extern uint32_t _stack_top[];

/* Picolibc/newlib initialization */
extern void __libc_init_array(void);
extern void __libc_fini_array(void);

/* User's main function */
extern int main(void);

/* Exit function */
extern void _exit(int status) __attribute__((noreturn));

/*
 * _start - Entry point
 *
 * Called by reset vector. Initializes BSS, data, and C library,
 * then calls main() and exits with its return value.
 */
void __attribute__((naked, noreturn, section(".text.startup"))) _start(void) {
    /* Set up stack pointer */
    asm volatile(
        "ldx #_stack_top\n\t"
        "txs\n\t"
    );
    
    /* Initialize BSS to zero */
    uint32_t *bss = _bss_start;
    while (bss < _bss_end) {
        *bss++ = 0;
    }
    
    /* Copy initialized data from ROM to RAM (if needed) */
    uint32_t *src = _data_load;
    uint32_t *dst = _data_start;
    if (src != dst) {
        while (dst < _data_end) {
            *dst++ = *src++;
        }
    }
    
    /* Initialize C library (calls constructors) */
    __libc_init_array();
    
    /* Call main */
    int ret = main();
    
    /* Call destructors and exit */
    __libc_fini_array();
    _exit(ret);
}

/*
 * Weak definitions in case picolibc doesn't provide them
 */
__attribute__((weak)) void __libc_init_array(void) {}
__attribute__((weak)) void __libc_fini_array(void) {}
