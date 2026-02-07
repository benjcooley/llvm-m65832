# Fix: FrameIndex + constant offset degenerates to IMPLICIT_DEF via disjoint OR

**Date**: 2026-02-06
**Author**: AI Agent
**Severity**: Critical
**Component**: Backend / Instruction Selection (M65832ISelDAGToDAG.cpp)

## Problem

### Symptoms
- `libc_testsuite.string` test failing: strcpy(b+1, c) called with R0=0 (NULL)
- `baremetal.test_string_suite` had 5 sub-test failures (down to 1 after fix)
- Assembly showed `; implicit-def: $r0` clobbering values before function calls
- Any getelementptr(alloca, small_constant_offset) used as a call arg was broken

### Root Cause
The LLVM DAG combiner transforms ADD(FrameIndex, small_constant) into a
**disjoint OR(FrameIndex, small_constant)** when frame alignment guarantees
no bits overlap (e.g., b is 4-byte aligned, so b|1 == b+1).

The M65832 instruction selector had no handling for OR(FrameIndex, constant).
The pattern matcher could not match it to any instruction, so the value
became IMPLICIT_DEF (undefined).

## Investigation

### Key Insight
LLVM SelectionDAG ISel processes nodes top-down (root to leaves). ADD/OR nodes
are selected BEFORE their FrameIndex operands. At selection time, FrameIndex
is not a GPR, so ADDI_GPR patterns don't match.

Added diagnostic fprintf to Select() to discover:
- ISD::OR = 192, ISD::ADD = 59, ISD::FrameIndex = 16
- GEP(alloca, 1) produced OR(FI, 1) not ADD(FI, 1)
- Zero ADD(FrameIndex, *) nodes existed; all were disjoint OR

## Solution

### Changes Made (M65832ISelDAGToDAG.cpp)

1. **Truncating store guard**: `if (ST->isTruncatingStore()) break;` prevents
   32-bit store optimizations from corrupting i8/i16 stores.

2. **ADD(FrameIndex, constant) -> LEA_FI**: In Select() ISD::ADD handler,
   emit LEA_FI(FI, offset) directly when operand is FrameIndex + constant.

3. **OR(FrameIndex, constant) -> LEA_FI**: In Select() ISD::OR handler,
   same logic for disjoint OR. This is the critical fix.

### Testing
```
Before: 157 passed, 19 skipped, 6 failed
After:  157 passed, 19 skipped, 5 failed (+1 fixed)

Fixed: libc_testsuite.string (FAIL -> PASS)
Improved: baremetal.test_string_suite (5 sub-failures -> 1)
```

Remaining 5 failures are pre-existing library-level issues.
