/* init.c - Baremetal initialization/finalization for M65832 */

#include <stddef.h>

/* Constructor/destructor array boundaries (provided by linker) */
extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);
extern void (*__fini_array_start[])(void);
extern void (*__fini_array_end[])(void);

/* Call all global constructors */
void __libc_init_array(void) {
    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; i++) {
        __init_array_start[i]();
    }
}

/* Call all global destructors */
void __libc_fini_array(void) {
    size_t count = __fini_array_end - __fini_array_start;
    /* Call in reverse order */
    for (size_t i = count; i > 0; i--) {
        __fini_array_start[i - 1]();
    }
}
