# Fix: Disjoint OR folded to ADD for pointer arithmetic

**Date**: 2026-01-31  
**Author**: AI Agent  
**Severity**: High  
**Component**: Instruction Selection

## Problem

### Symptoms
- `vsprintf`/`stdio` builds would hang or fail during instruction selection at `-O2`.
- The issue was tied to a `%s` path in the printf family.

### Root Cause
- The DAG contained `ISD::OR` with the **disjoint** flag for pointer arithmetic.
- The M65832 selector did not treat that as equivalent to `ADD`, so address
  patterns could not match, leading to selection failure/spin.

## Investigation

### How the Issue Was Isolated
1. Reduced `stdio.c` to a minimal `va_arg` + `*s++` loop.
2. Confirmed `llc` could lower the IR, while `clang -S/-c` at `-O2` hung.
3. Inspected ISel input/output for the `%s` loop and observed disjoint OR.

### Debug Output
```
... disjoint OR used for pointer arithmetic ...
```

## Solution

### Changes Made
- In `M65832ISelDAGToDAG.cpp`, treat `ISD::OR` with `hasDisjoint()` as `ADD`
  for `i32` and replace the node early in `Select()`.

### Why This Fix Works
Disjoint OR is semantically equivalent to ADD for non-overlapping bitfields.
By canonicalizing to ADD, the address selection patterns are able to match
and the selector no longer spins.

### Testing
- `repro_stdio_hang.c` now compiles at `-O2`.
- `m65832-stdlib/libc/src/stdio/stdio.c` now compiles at `-O2`.

## Recommendations
- When debugging selector hangs, check for disjoint OR in pointer arithmetic.
- Use `-print-isel-input` and minimal C repros to confirm the DAG form.

## References
- `llvm/lib/Target/M65832/M65832ISelDAGToDAG.cpp`
