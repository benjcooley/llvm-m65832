//===-- M65832 Baremetal I/O hooks for LLVM libc --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides the LLVM libc I/O hooks for M65832 baremetal.
// It calls the platform UART functions which have no LLVM dependency.
//
//===----------------------------------------------------------------------===//

#include "src/__support/macros/config.h"
#include <stddef.h>

// Forward declarations of platform functions (defined in m65832/emu/platform/)
extern "C" {
size_t uart_write(const char *buf, size_t len);
size_t uart_read(char *buf, size_t len);
}

namespace LIBC_NAMESPACE_DECL {

// Cookie structure - can be empty for simple UART I/O
struct __llvm_libc_stdio_cookie {};

} // namespace LIBC_NAMESPACE_DECL

// Provide the cookie symbols LLVM libc expects
extern "C" {

LIBC_NAMESPACE::__llvm_libc_stdio_cookie __llvm_libc_stdin_cookie;
LIBC_NAMESPACE::__llvm_libc_stdio_cookie __llvm_libc_stdout_cookie;
LIBC_NAMESPACE::__llvm_libc_stdio_cookie __llvm_libc_stderr_cookie;

// LLVM libc calls this for stdio write operations
ssize_t __llvm_libc_stdio_write(void *cookie, const char *buf, size_t size) {
    (void)cookie;  // All output goes to UART
    return static_cast<ssize_t>(uart_write(buf, size));
}

// LLVM libc calls this for stdio read operations
ssize_t __llvm_libc_stdio_read(void *cookie, char *buf, size_t size) {
    (void)cookie;  // All input comes from UART
    return static_cast<ssize_t>(uart_read(buf, size));
}

} // extern "C"
