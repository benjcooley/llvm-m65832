/* syscalls.c - Picolibc system call stubs for M65832 baremetal
 *
 * These functions provide the minimal system interface required by picolibc.
 * For baremetal operation, most syscalls are stubs that return errors.
 */

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>

/* UART registers for console I/O (memory-mapped at 0x00FFF100) */
#define UART_STATUS    (*(volatile uint32_t *)0x00FFF100)
#define UART_TX_DATA   (*(volatile uint32_t *)0x00FFF104)
#define UART_RX_DATA   (*(volatile uint32_t *)0x00FFF108)

/* Status bits */
#define UART_TX_READY  0x01
#define UART_RX_AVAIL  0x02

/* ============================================================================
 * STDIO Support - stdin/stdout/stderr via UART
 * ========================================================================= */

static int uart_putc(char c, FILE *file) {
    (void)file;
    /* Wait for TX ready */
    while (!(UART_STATUS & UART_TX_READY))
        ;
    UART_TX_DATA = (uint32_t)(unsigned char)c;
    return (unsigned char)c;
}

static int uart_getc(FILE *file) {
    (void)file;
    /* Wait for RX available */
    while (!(UART_STATUS & UART_RX_AVAIL))
        ;
    return (int)(UART_RX_DATA & 0xFF);
}

/* Create the stdio FILE structure */
static FILE __stdio = FDEV_SETUP_STREAM(uart_putc, uart_getc, NULL, _FDEV_SETUP_RW);

/* Define stdin, stdout, stderr */
#ifdef __strong_reference
#define STDIO_ALIAS(x) __strong_reference(stdin, x);
#else
#define STDIO_ALIAS(x) FILE * const x = &__stdio;
#endif

FILE * const stdin = &__stdio;
STDIO_ALIAS(stdout);
STDIO_ALIAS(stderr);

/* ============================================================================
 * Compiler Runtime Support (64-bit arithmetic)
 * These are normally provided by compiler-rt/libgcc
 * ========================================================================= */

/* 64-bit multiplication */
uint64_t __muldi3(uint64_t a, uint64_t b) {
    uint32_t al = (uint32_t)a;
    uint32_t ah = (uint32_t)(a >> 32);
    uint32_t bl = (uint32_t)b;
    uint32_t bh = (uint32_t)(b >> 32);
    
    uint64_t result = (uint64_t)al * bl;
    result += ((uint64_t)al * bh) << 32;
    result += ((uint64_t)ah * bl) << 32;
    return result;
}

/* 64-bit unsigned division */
uint64_t __udivdi3(uint64_t num, uint64_t den) {
    if (den == 0) return 0;  /* Avoid div by zero */
    
    uint64_t quot = 0;
    int bits = 64;
    
    /* Simple bit-by-bit division */
    while (bits-- > 0) {
        quot <<= 1;
        if (num >= den) {
            num -= den;
            quot |= 1;
        }
        den >>= 1;
    }
    return quot;
}

/* 64-bit signed division */
int64_t __divdi3(int64_t a, int64_t b) {
    int neg = 0;
    if (a < 0) { a = -a; neg = !neg; }
    if (b < 0) { b = -b; neg = !neg; }
    uint64_t result = __udivdi3((uint64_t)a, (uint64_t)b);
    return neg ? -(int64_t)result : (int64_t)result;
}

/* 64-bit unsigned modulo */
uint64_t __umoddi3(uint64_t num, uint64_t den) {
    return num - __udivdi3(num, den) * den;
}

/* 64-bit signed modulo */
int64_t __moddi3(int64_t a, int64_t b) {
    int neg = 0;
    if (a < 0) { a = -a; neg = 1; }
    if (b < 0) { b = -b; }
    uint64_t result = __umoddi3((uint64_t)a, (uint64_t)b);
    return neg ? -(int64_t)result : (int64_t)result;
}

/* ============================================================================
 * System Calls
 * ========================================================================= */

/* Heap management */
extern char _end[];           /* Set by linker - end of BSS */
extern char _heap_end[];      /* Set by linker - end of heap */
static char *heap_ptr = 0;

/*
 * _sbrk - Increase program data space (heap)
 * 
 * This is the only memory allocation primitive - malloc uses it.
 */
void *_sbrk(ptrdiff_t incr) {
    char *prev_heap;
    
    if (heap_ptr == 0) {
        heap_ptr = _end;
    }
    
    prev_heap = heap_ptr;
    
    /* Check for heap overflow */
    if (heap_ptr + incr > _heap_end) {
        errno = ENOMEM;
        return (void *)-1;
    }
    
    heap_ptr += incr;
    return prev_heap;
}

/*
 * _write - Write to a file descriptor
 *
 * For baremetal, only stdout (1) and stderr (2) are supported via UART.
 */
ssize_t _write(int fd, const void *buf, size_t len) {
    if (fd != 1 && fd != 2) {
        errno = EBADF;
        return -1;
    }
    
    const char *p = buf;
    for (size_t i = 0; i < len; i++) {
        /* Wait for transmit ready */
        while (!(UART_STATUS & UART_TX_READY))
            ;
        UART_TX_DATA = (uint32_t)(unsigned char)p[i];
    }
    
    return (ssize_t)len;
}

/*
 * _read - Read from a file descriptor
 *
 * For baremetal, only stdin (0) is supported via UART.
 */
ssize_t _read(int fd, void *buf, size_t len) {
    if (fd != 0) {
        errno = EBADF;
        return -1;
    }
    
    char *p = buf;
    size_t i;
    for (i = 0; i < len; i++) {
        /* Wait for receive ready */
        while (!(UART_STATUS & UART_RX_AVAIL))
            ;
        p[i] = (char)(UART_RX_DATA & 0xFF);
        
        /* Echo and handle line endings */
        if (p[i] == '\r' || p[i] == '\n') {
            p[i] = '\n';
            i++;
            break;
        }
    }
    
    return (ssize_t)i;
}

/*
 * _exit - Terminate the program
 */
void __attribute__((noreturn)) _exit(int status) {
    /* For baremetal, we just store the exit status and halt.
     * Use a volatile write to prevent optimization. */
    volatile int *exit_code = (volatile int *)0xFFFFFFFC;
    *exit_code = status;
    
    /* Stop the processor */
    asm volatile("stp");
    
    /* Never returns */
    for(;;) { }
    __builtin_unreachable();
}

/*
 * _close - Close a file descriptor
 */
int _close(int fd) {
    if (fd >= 0 && fd <= 2) {
        return 0;
    }
    errno = EBADF;
    return -1;
}

/*
 * _fstat - Get file status
 */
int _fstat(int fd, struct stat *st) {
    if (fd >= 0 && fd <= 2) {
        st->st_mode = S_IFCHR;
        return 0;
    }
    errno = EBADF;
    return -1;
}

/*
 * _isatty - Check if fd is a terminal
 */
int _isatty(int fd) {
    if (fd >= 0 && fd <= 2) {
        return 1;
    }
    errno = EBADF;
    return 0;
}

/*
 * _lseek - Seek in a file
 */
off_t _lseek(int fd, off_t offset, int whence) {
    (void)fd;
    (void)offset;
    (void)whence;
    errno = ESPIPE;
    return -1;
}

/*
 * _kill - Send signal to process
 */
int _kill(pid_t pid, int sig) {
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

/*
 * _getpid - Get process ID
 */
pid_t _getpid(void) {
    return 1;
}
