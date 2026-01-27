/* syscalls.c - Newlib system call stubs for M65832 baremetal
 *
 * These functions provide the minimal system interface required by newlib.
 * For baremetal operation, most syscalls are stubs that return errors.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/times.h>
#include <errno.h>

/* UART registers for console I/O */
#define UART_BASE    0xFFE0
#define UART_DATA    (*(volatile unsigned char *)(UART_BASE + 0))
#define UART_STATUS  (*(volatile unsigned char *)(UART_BASE + 1))
#define UART_TX_READY 0x01
#define UART_RX_READY 0x02

/* Heap management */
extern char _end;           /* Set by linker - end of BSS */
extern char _heap_end;      /* Set by linker - end of heap */
static char *heap_ptr = 0;

/* Errno - required by newlib */
#undef errno
extern int errno;

/*
 * _sbrk - Increase program data space (heap)
 * 
 * This is the only memory allocation primitive - malloc uses it.
 */
void *_sbrk(int incr) {
    char *prev_heap;
    
    if (heap_ptr == 0) {
        heap_ptr = &_end;
    }
    
    prev_heap = heap_ptr;
    
    /* Check for heap overflow */
    if (heap_ptr + incr > &_heap_end) {
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
int _write(int fd, char *buf, int len) {
    if (fd != 1 && fd != 2) {
        errno = EBADF;
        return -1;
    }
    
    for (int i = 0; i < len; i++) {
        /* Wait for transmit ready */
        while (!(UART_STATUS & UART_TX_READY))
            ;
        UART_DATA = buf[i];
    }
    
    return len;
}

/*
 * _read - Read from a file descriptor
 *
 * For baremetal, only stdin (0) is supported via UART.
 */
int _read(int fd, char *buf, int len) {
    if (fd != 0) {
        errno = EBADF;
        return -1;
    }
    
    int i;
    for (i = 0; i < len; i++) {
        /* Wait for receive ready */
        while (!(UART_STATUS & UART_RX_READY))
            ;
        buf[i] = UART_DATA;
        
        /* Echo and handle line endings */
        if (buf[i] == '\r' || buf[i] == '\n') {
            buf[i] = '\n';
            i++;
            break;
        }
    }
    
    return i;
}

/*
 * _exit - Terminate the program
 */
void _exit(int status) {
    /* Store exit status in A register */
    asm volatile("lda %0" : : "r"(status));
    /* Stop the processor */
    asm volatile("stp");
    /* Never returns */
    while (1)
        ;
}

/*
 * _close - Close a file descriptor
 *
 * Stub - always succeeds for stdin/stdout/stderr
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
 *
 * Stub - reports character device for stdin/stdout/stderr
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
 *
 * stdin/stdout/stderr are always terminals
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
 *
 * Stub - not supported for console
 */
int _lseek(int fd, int offset, int whence) {
    (void)fd;
    (void)offset;
    (void)whence;
    errno = ESPIPE;
    return -1;
}

/*
 * _kill - Send signal to process
 *
 * Stub - no processes in baremetal
 */
int _kill(int pid, int sig) {
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

/*
 * _getpid - Get process ID
 *
 * Stub - always returns 1
 */
int _getpid(void) {
    return 1;
}

/*
 * _times - Get process times
 *
 * Stub - not supported
 */
clock_t _times(struct tms *buf) {
    (void)buf;
    return (clock_t)-1;
}

/*
 * _stat - Get file status by name
 *
 * Stub - no filesystem
 */
int _stat(const char *file, struct stat *st) {
    (void)file;
    (void)st;
    errno = ENOENT;
    return -1;
}

/*
 * _link - Create a link
 *
 * Stub - no filesystem
 */
int _link(char *old, char *new) {
    (void)old;
    (void)new;
    errno = EMLINK;
    return -1;
}

/*
 * _unlink - Remove a file
 *
 * Stub - no filesystem
 */
int _unlink(char *name) {
    (void)name;
    errno = ENOENT;
    return -1;
}

/*
 * _fork - Create a new process
 *
 * Stub - no processes
 */
int _fork(void) {
    errno = EAGAIN;
    return -1;
}

/*
 * _wait - Wait for child process
 *
 * Stub - no processes
 */
int _wait(int *status) {
    (void)status;
    errno = ECHILD;
    return -1;
}

/*
 * _execve - Execute a program
 *
 * Stub - no processes
 */
int _execve(char *name, char **argv, char **env) {
    (void)name;
    (void)argv;
    (void)env;
    errno = ENOMEM;
    return -1;
}

/*
 * _open - Open a file
 *
 * Stub - no filesystem
 */
int _open(const char *name, int flags, int mode) {
    (void)name;
    (void)flags;
    (void)mode;
    errno = ENOENT;
    return -1;
}
