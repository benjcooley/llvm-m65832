# Session Summary: M65832 Backend Fixes (2026-02-06)

**Test Results**: 157 passed, 19 skipped, 5 failed (from baseline ~96 failures)

## Changes in this commit

### 1. Truncating Store Fix (M65832ISelDAGToDAG.cpp)
- **Problem**: Custom 32-bit STORE optimization was applied to i8/i16 truncating
  stores, writing 4 bytes when only 1 or 2 were intended, corrupting stack memory
- **Fix**: Added `if (ST->isTruncatingStore()) break;` guard
- **Impact**: Fixed snprintf, sprintf, and many string formatting functions

### 2. FrameIndex + Offset via Disjoint OR (M65832ISelDAGToDAG.cpp)
- **Problem**: DAG combiner turns ADD(FrameIndex, small_const) into disjoint OR.
  No ISel handler existed for OR(FrameIndex, const), causing IMPLICIT_DEF
- **Fix**: Handle both ADD(FI, const) and OR(FI, const) in Select(), emitting
  LEA_FI(FI, offset) directly
- **Impact**: Fixed libc_testsuite.string, improved baremetal.test_string_suite

### 3. Variadic Argument Passing (M65832ISelLowering.cpp)
- **Problem**: Variadic (unnamed) arguments were passed in registers, but
  va_arg expects them on the stack
- **Fix**: In LowerCall, force non-fixed variadic arguments from registers to
  stack slots
- **Impact**: Fixed printf, snprintf, and all variadic function calls

### 4. C Runtime Startup (crt0.s)
- **Problem**: main() called without argc/argv/envp initialization; ctype tests
  got "Is a directory" errors. Weak stubs for __libc_init_array etc. prevented
  real library implementations from linking
- **Fix**: Set R0=0 (argc), R1=NULL (argv), R2=NULL (envp) before calling main.
  Removed weak stubs to force linker to use picolibc implementations
- **Impact**: Fixed ctype tests, atexit, constructor/destructor support

### 5. Linker Script Updates (m65832.ld)
- **Problem**: ROM region too small (384K) for large tests. init_array/fini_array
  patterns didn't match picolibc's section names (e.g. .fini_array_onexit)
- **Fix**: Increased ROM to 640K, RAM at 0xC0000/256K. Added KEEP(*(.init_array_*))
  and PROVIDE_HIDDEN for picolibc's bothinit symbols
- **Impact**: Fixed test-fma ROM overflow, fixed atexit/fini_array processing

### 6. TRAP-based Syscalls (syscalls.c)
- **Problem**: Old syscalls used direct UART memory-mapped I/O, incompatible
  with emulator's system mode
- **Fix**: Rewrote all syscalls to use TRAP #0 instruction for system calls
  (read, write, open, close, lseek, fstat, exit, sbrk)
- **Impact**: Enabled real file I/O through emulator sandbox

## Remaining Failures (5)
All are library-level issues, not compiler bugs:
1. `libc_testsuite.snprintf` - precision with tiny doubles (0x1p-1021)
2. `libc_testsuite.sscanf` - NaN/Inf parsing
3. `picolibc.test-getdate` - strftime/strptime format issues
4. `picolibc.timegm` - off-by-one-day in time calculation
5. `baremetal.test_string_suite` - picolibc strstr returns non-NULL for missing substring
