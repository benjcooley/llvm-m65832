/* syscalls.c - Picolibc system call stubs for M65832 baremetal
 *
 * These functions provide the minimal system interface required by picolibc.
 * For baremetal operation, most syscalls are stubs that return errors.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

/* UART registers for console I/O */
#define UART_BASE    0xFFE0
#define UART_DATA    (*(volatile unsigned char *)(UART_BASE + 0))
#define UART_STATUS  (*(volatile unsigned char *)(UART_BASE + 1))
#define UART_TX_READY 0x01
#define UART_RX_READY 0x02

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
        UART_DATA = p[i];
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
        while (!(UART_STATUS & UART_RX_READY))
            ;
        p[i] = UART_DATA;
        
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
    /* Store exit status in A register */
    asm volatile("lda %0" : : "r"(status));
    /* Stop the processor */
    asm volatile("stp");
    /* Never returns */
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
