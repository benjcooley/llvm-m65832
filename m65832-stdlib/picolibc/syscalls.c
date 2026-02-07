/* syscalls.c - Picolibc system call stubs for M65832
 *
 * These functions provide the system interface required by picolibc.
 * All I/O uses TRAP #0 syscalls which are handled by the emulator's
 * system layer (--system --sandbox mode).
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
#include <fcntl.h>
#include <stdarg.h>

/* ============================================================================
 * TRAP-based Syscall Interface
 *
 * The M65832 TRAP #0 instruction invokes the system layer's syscall handler.
 * Syscall number goes in R0, arguments in R1-R5, return value in R0.
 * ========================================================================= */

#define M65832_SYS_EXIT     1
#define M65832_SYS_READ     3
#define M65832_SYS_WRITE    4
#define M65832_SYS_OPEN     5
#define M65832_SYS_CLOSE    6
#define M65832_SYS_LSEEK    19
#define M65832_SYS_GETPID   20
#define M65832_SYS_FSTAT    108
#define M65832_SYS_EXIT_GRP 248

static inline long __syscall0(long n) {
    register long r0 __asm__("r0") = n;
    __asm__ volatile(".byte 0x02, 0x40, 0x00" : "+r"(r0) : : "memory");
    return r0;
}

static inline long __syscall1(long n, long a1) {
    register long r0 __asm__("r0") = n;
    register long r1 __asm__("r1") = a1;
    __asm__ volatile(".byte 0x02, 0x40, 0x00" : "+r"(r0) : "r"(r1) : "memory");
    return r0;
}

static inline long __syscall2(long n, long a1, long a2) {
    register long r0 __asm__("r0") = n;
    register long r1 __asm__("r1") = a1;
    register long r2 __asm__("r2") = a2;
    __asm__ volatile(".byte 0x02, 0x40, 0x00" : "+r"(r0) : "r"(r1), "r"(r2) : "memory");
    return r0;
}

static inline long __syscall3(long n, long a1, long a2, long a3) {
    register long r0 __asm__("r0") = n;
    register long r1 __asm__("r1") = a1;
    register long r2 __asm__("r2") = a2;
    register long r3 __asm__("r3") = a3;
    __asm__ volatile(".byte 0x02, 0x40, 0x00" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3) : "memory");
    return r0;
}

static inline long __syscall_ret(long r) {
    if (r < 0 && r > -4096) {
        errno = (int)(-r);
        return -1;
    }
    return r;
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
 * _write - Write to a file descriptor (via TRAP syscall)
 */
ssize_t _write(int fd, const void *buf, size_t len) {
    return __syscall_ret(__syscall3(M65832_SYS_WRITE, fd, (long)buf, (long)len));
}

/*
 * _read - Read from a file descriptor (via TRAP syscall)
 */
ssize_t _read(int fd, void *buf, size_t len) {
    return __syscall_ret(__syscall3(M65832_SYS_READ, fd, (long)buf, (long)len));
}

/*
 * _exit - Terminate the program (via TRAP syscall)
 *
 * Sends SYS_EXIT_GRP and SYS_EXIT to the system layer.
 * The system layer sets cpu->exit_code and halts execution.
 */
void __attribute__((noreturn)) _exit(int status) {
    __syscall1(M65832_SYS_EXIT_GRP, status);
    __syscall1(M65832_SYS_EXIT, status);
    __builtin_unreachable();
}

/*
 * _open - Open a file (via TRAP syscall)
 */
int _open(const char *path, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }
    return (int)__syscall_ret(__syscall3(M65832_SYS_OPEN, (long)path, (long)flags, (long)mode));
}

/*
 * _close - Close a file descriptor (via TRAP syscall)
 */
int _close(int fd) {
    return (int)__syscall_ret(__syscall1(M65832_SYS_CLOSE, fd));
}

/*
 * _fstat - Get file status (via TRAP syscall)
 */
int _fstat(int fd, struct stat *st) {
    return (int)__syscall_ret(__syscall2(M65832_SYS_FSTAT, fd, (long)st));
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
 * _lseek - Seek in a file (via TRAP syscall)
 */
off_t _lseek(int fd, off_t offset, int whence) {
    return (off_t)__syscall_ret(__syscall3(M65832_SYS_LSEEK, fd, (long)offset, (long)whence));
}

/*
 * _kill - Send signal to process (stub)
 */
int _kill(pid_t pid, int sig) {
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

/*
 * _getpid - Get process ID (via TRAP syscall)
 */
pid_t _getpid(void) {
    return (pid_t)__syscall0(M65832_SYS_GETPID);
}

/* ============================================================================
 * Additional stubs for picolibc test compatibility
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
