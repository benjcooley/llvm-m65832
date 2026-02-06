/* syscalls.c - Picolibc system call stubs for M65832 baremetal
 *
 * These functions provide the minimal system interface required by picolibc.
 * For baremetal operation, most syscalls are stubs that return errors.
 *
 * NOTE: stdin/stdout/stderr are defined by picolibc's m65832_iob.c (in libc.a).
 *       Do NOT define them here or you'll get duplicate symbol errors.
 *
 * NOTE: Compiler-rt functions (__muldi3, __udivdi3, etc.) are provided by
 *       libcompiler_rt.a. Do NOT define them here.
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

/* ============================================================================
 * Additional filesystem/signal stubs for picolibc test compatibility
 * ========================================================================= */

int _unlink(const char *path) {
    (void)path;
    errno = ENOENT;
    return -1;
}

int _link(const char *oldpath, const char *newpath) {
    (void)oldpath;
    (void)newpath;
    errno = EMLINK;
    return -1;
}

int _stat(const char *path, struct stat *st) {
    (void)path;
    (void)st;
    errno = ENOENT;
    return -1;
}

int _rename(const char *oldpath, const char *newpath) {
    (void)oldpath;
    (void)newpath;
    errno = ENOENT;
    return -1;
}

int _open(const char *path, int flags, ...) {
    (void)path;
    (void)flags;
    errno = ENOENT;
    return -1;
}

int _wait(int *status) {
    (void)status;
    errno = ECHILD;
    return -1;
}

int _fork(void) {
    errno = EAGAIN;
    return -1;
}

int _execve(const char *name, char *const argv[], char *const env[]) {
    (void)name;
    (void)argv;
    (void)env;
    errno = ENOMEM;
    return -1;
}
