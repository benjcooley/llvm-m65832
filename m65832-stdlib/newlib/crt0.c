/* crt0.c - C Runtime startup for M65832 with Newlib
 *
 * This is the entry point called by the reset vector.
 * It initializes the C runtime environment and calls main().
 */

/* Linker-provided symbols */
extern unsigned int _bss_start;
extern unsigned int _bss_end;
extern unsigned int _data_start;
extern unsigned int _data_end;
extern unsigned int _data_load;

/* Newlib initialization */
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
void __attribute__((naked, section(".text.startup"))) _start(void) {
    /* Set up stack pointer - linker defines _stack_top */
    asm volatile(
        "ldx #_stack_top\n\t"
        "txs\n\t"
    );
    
    /* Initialize BSS to zero */
    unsigned int *bss = &_bss_start;
    while (bss < &_bss_end) {
        *bss++ = 0;
    }
    
    /* Copy initialized data from ROM to RAM (if needed) */
    /* For RAM-only execution, _data_load == _data_start */
    unsigned int *src = &_data_load;
    unsigned int *dst = &_data_start;
    if (src != dst) {
        while (dst < &_data_end) {
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
 * Dummy init/fini arrays if not provided by linker
 */
__attribute__((weak)) void __libc_init_array(void) {
    /* Weak definition - real one comes from newlib */
}

__attribute__((weak)) void __libc_fini_array(void) {
    /* Weak definition - real one comes from newlib */
}
