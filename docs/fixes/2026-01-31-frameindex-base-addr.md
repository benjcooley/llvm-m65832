# Fix: TargetFrameIndex for base+offset addressing

**Date**: 2026-01-31  
**Author**: AI Agent  
**Severity**: High  
**Component**: Instruction Selection

## Problem

### Symptoms
- `clang -O2` on `stdio.c` or the reduced `%s` varargs loop would hang.
- `llc` could lower the IR, but clang codegen would spin.

### Root Cause
- `selectAddr` returned a raw `FrameIndex` as the base when used in
  `ADD/OR(base, const)` forms.
- `ADDRri` expects a *target* frame index (`TargetFrameIndex`), and
  the mismatch caused address selection to fail and repeat.

## Investigation

### How the Issue Was Isolated
1. Reduced `stdio.c` to a minimal `va_arg` + `*s++` loop.
2. Verified that `llc` completed but `clang -S` hung.
3. Confirmed the address pattern was `ADD(FrameIndex, const)` / `OR`.

### Debug Output
```
clang -S -O2 hangs; llc -O2 succeeds on same IR
```

## Solution

### Changes Made
- In `selectAddr`, convert base `FrameIndex` to `TargetFrameIndex` before
  forming the base+offset pair for both `ADD` and `OR`.

### Why This Fix Works
`TargetFrameIndex` is the form expected by the `ADDRri` complex pattern.
By normalizing the base early, instruction selection can match the address
and no longer spins.

### Testing
- `repro_stdio_hang.c` compiles at `-O2`.
- `m65832-stdlib/libc/src/stdio/stdio.c` compiles at `-O2`.

## Recommendations
- Treat all base+offset frame addresses as `TargetFrameIndex` in selector helpers.
- When clang hangs but llc succeeds, inspect address forms in ISel.

## References
- `llvm/lib/Target/M65832/M65832ISelDAGToDAG.cpp`
