//===-- M65832 Baremetal syscall hooks for LLVM libc ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides the LLVM libc system hooks for M65832 baremetal.
// It calls the platform system functions which have no LLVM dependency.
//
//===----------------------------------------------------------------------===//

#include "src/__support/macros/config.h"
#include <stddef.h>

// Forward declarations of platform functions (defined in m65832/emu/platform/)
extern "C" {
void *sys_sbrk(int incr);
void sys_exit(int status) __attribute__((noreturn));
void sys_abort(void) __attribute__((noreturn));
}

extern "C" {

// LLVM libc calls _sbrk for heap allocation (used by malloc)
void *_sbrk(int incr) {
    return sys_sbrk(incr);
}

// LLVM libc calls _exit for program termination
void _exit(int status) {
    sys_exit(status);
}

// LLVM libc calls abort() 
void abort(void) {
    sys_abort();
}

// Minimal stubs for other required functions
int _getpid(void) {
    return 1;  // Always PID 1 on baremetal
}

int _kill(int pid, int sig) {
    (void)pid;
    (void)sig;
    return -1;  // Not supported
}

int _isatty(int fd) {
    return (fd >= 0 && fd <= 2) ? 1 : 0;
}

int _fstat(int fd, void *st) {
    (void)fd;
    (void)st;
    return -1;  // Minimal implementation
}

int _close(int fd) {
    (void)fd;
    return 0;
}

int _lseek(int fd, int offset, int whence) {
    (void)fd;
    (void)offset;
    (void)whence;
    return -1;
}

} // extern "C"
