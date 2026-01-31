# Fix: OR(FrameIndex, const) address selection for memmove

**Date**: 2026-01-31  
**Author**: AI Agent  
**Severity**: High  
**Component**: Instruction Selection

## Problem

### Symptoms
- `memmove` tests produced `@<noreg>` in assembly.
- Stack-local array indexing could fail in `-O1/-O2` with inlined memcpy/memmove.

### Root Cause
- The selector only handled `ADD(FrameIndex, const)` in `selectAddr`.
- `OR(FrameIndex, const)` (used for pointer arithmetic in some cases) was not
  recognized, so the base register was missing and resulted in `@<noreg>`.

## Investigation

### How the Issue Was Isolated
1. Reduced to a minimal stack-local array test.
2. Confirmed `@<noreg>` emitted for an address based on a frame index.
3. Observed DAG using `ISD::OR` with a constant offset.

### Debug Output
```
@<noreg> generated for stack-based address
```

## Solution

### Changes Made
- In `M65832ISelDAGToDAG.cpp`, teach `selectAddr` to recognize
  `OR(base, const)` and treat it as base+offset addressing.

### Why This Fix Works
`ADDRri` expects a base register and a constant offset. By accepting OR forms,
the selector can map the address to the correct base+offset pair and emit
valid code.

### Testing
- Added `llvm/test/CodeGen/M65832/stack-array-or-addr.ll`.
- Minimal C repro now compiles correctly.

## Recommendations
- Always normalize pointer arithmetic patterns in `selectAddr` for ADD/OR.
- Add regression tests that check for `@<noreg>` in codegen.

## References
- `llvm/lib/Target/M65832/M65832ISelDAGToDAG.cpp`
- `llvm/test/CodeGen/M65832/stack-array-or-addr.ll`
