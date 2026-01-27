# M65832 Picolibc Integration Plan

## Overview

Use picolibc as our C standard library for baremetal M65832. Picolibc is:
- Actively maintained (commits as recent as April 2025)
- Based on newlib but with modern build system (Meson)
- Designed specifically for embedded 32/64-bit systems
- Smaller footprint with tinystdio option

## Staged Approach

### Stage 1: Nano (Minimal Libc)
Get a minimal working libc first using our existing custom implementation:
- String functions (memcpy, strcpy, strcmp, strlen, etc.)
- Memory functions (memset, memmove)
- Ctype functions (isalpha, isdigit, toupper, etc.)
- Basic stdio (puts, putchar, getchar)
- Stdlib basics (abs, atoi, malloc/free stubs)

**Goal:** Verify compiler and runtime work correctly with minimal dependencies.

### Stage 2: Full Picolibc
Once nano works, switch to full picolibc:
- Complete stdio with printf/scanf
- Full stdlib with proper malloc
- Math library
- All standard headers

## Version

**Picolibc Version:** 1.8.6 (or latest stable tag)

Repository: https://github.com/picolibc/picolibc

## Directory Structure

```
/Users/benjamincooley/projects/
├── llvm-m65832/          # LLVM/Clang toolchain
├── M65832/               # Emulator and platform
├── picolibc/             # Cloned picolibc source (auto-cloned by build)
└── picolibc-build-m65832/  # Picolibc build directory
```

## Phase 1: Setup Infrastructure

### 1.1 Clone Picolibc
```bash
cd /Users/benjamincooley/projects
git clone --branch 1.8.6 --depth 1 https://github.com/picolibc/picolibc.git
```

### 1.2 Create M65832 Cross File

Meson uses cross-files to configure cross-compilation. Create `cross-m65832.txt`:

```ini
[binaries]
c = '/Users/benjamincooley/projects/llvm-m65832/build/bin/clang'
ar = '/Users/benjamincooley/projects/llvm-m65832/build/bin/llvm-ar'
as = '/Users/benjamincooley/projects/llvm-m65832/build/bin/clang'
strip = '/Users/benjamincooley/projects/llvm-m65832/build/bin/llvm-strip'

[host_machine]
system = 'none'
cpu_family = 'm65832'
cpu = 'm65832'
endian = 'little'

[properties]
c_args = ['-target', 'm65832-elf', '-ffreestanding']
c_link_args = ['-target', 'm65832-elf', '-nostdlib']
skip_sanity_check = true
```

### 1.3 Build Configuration

```bash
cd /Users/benjamincooley/projects
mkdir picolibc-build-m65832 && cd picolibc-build-m65832

meson setup \
    --cross-file ../llvm-m65832/m65832-stdlib/picolibc/cross-m65832.txt \
    --prefix=/Users/benjamincooley/projects/m65832-sysroot \
    -Dmultilib=false \
    -Dpicolib=false \
    -Dpicocrt=false \
    -Dsemihost=false \
    -Dspecsdir=none \
    -Dtests=false \
    ../picolibc

meson compile
meson install
```

## Phase 2: System Support

### 2.1 Syscalls Implementation

Picolibc needs these syscall stubs for baremetal:
- `_read()` - UART input
- `_write()` - UART output  
- `_sbrk()` - Heap management
- `_exit()` - Program termination
- Plus stub implementations for `_close`, `_fstat`, `_isatty`, `_lseek`, `_kill`, `_getpid`

### 2.2 Startup Code (crt0)

Custom crt0 that:
1. Sets up stack pointer
2. Initializes BSS to zero
3. Copies initialized data (if ROM to RAM)
4. Calls `__libc_init_array()` (constructors)
5. Calls `main()`
6. Calls `__libc_fini_array()` (destructors)
7. Calls `_exit()` with return value

### 2.3 Linker Script

Define memory layout for M65832:
- ROM/Flash at 0x1000 (code)
- RAM at 0xC000 (data, BSS, heap)
- Stack at 0xFFFF (grows down)

## Phase 3: Testing

### 3.1 Test Organization

```
M65832/emu/c_tests/
├── baremetal/
│   ├── core/           # Pure C tests (no libc)
│   └── picolibc/       # Tests using picolibc
├── regression/         # Compiler regression tests
```

### 3.2 Using Picolibc's Test Suite

Picolibc has a comprehensive test suite. We run it on our emulator using meson's `exe_wrapper`:

**Cross file configuration:**
```ini
[binaries]
exe_wrapper = ['path/to/run-m65832.sh']
```

**Wrapper script (`run-m65832.sh`):**
```bash
#!/bin/bash
EMU="/path/to/m65832emu"
exec "$EMU" "$1" -n 1000000
```

**Running tests:**
```bash
cd picolibc-build-m65832
meson test                    # Run all tests
meson test --list             # List available tests
meson test string             # Run specific test
meson test -v                 # Verbose output
```

### 3.3 Test Categories in Picolibc

Picolibc includes tests for:
- **string** - String functions (memcpy, strcpy, strcmp, etc.)
- **stdlib** - Standard library (malloc, atoi, qsort, etc.)
- **stdio** - Input/output (printf, scanf, fopen, etc.)
- **math** - Math library (sin, cos, sqrt, etc.)
- **ctype** - Character classification
- **time** - Time functions
- **locale** - Locale support

### 3.4 Expected Test Behavior

- Tests return 0 on success, non-zero on failure
- Emulator instruction limit (1M) should be sufficient for most tests
- Some tests may need semihosting for command-line args (future enhancement)

## Advantages over Newlib

1. **Active development** - Regular commits, responsive maintainer
2. **Modern build system** - Meson is cleaner than autotools
3. **Smaller footprint** - tinystdio option for minimal printf
4. **Better embedded focus** - Designed for constrained systems
5. **Cleaner codebase** - Modernized from newlib

## Files to Create

- [x] `m65832-stdlib/picolibc/cross-m65832.txt` - Meson cross file
- [x] `m65832-stdlib/picolibc/syscalls.c` - System call implementations
- [x] `m65832-stdlib/picolibc/crt0.c` - Startup code
- [x] `m65832-stdlib/picolibc/m65832.ld` - Linker script
- [x] `m65832-stdlib/scripts/build_picolibc.sh` - Build automation

## References

- [Picolibc GitHub](https://github.com/picolibc/picolibc)
- [Picolibc Build Documentation](https://github.com/picolibc/picolibc/blob/main/doc/build.md)
- [Meson Cross Compilation](https://mesonbuild.com/Cross-compilation.html)
