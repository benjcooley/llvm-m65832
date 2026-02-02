# Fix: 64-bit divide/mod runtime (compiler-rt)

**Date**: 2026-02-01  
**Author**: AI Agent  
**Severity**: High  
**Component**: Library / compiler-rt

## Problem

### Symptoms
- Core test failure:
  - `baremetal/core/test_64bit_divide.c` returned `2` or `8`
- Minimal repro:
  - `debug_udiv64_r2.c` (expected `100000`)
  - `debug_umod64_r8.c` (expected `7`)

### Root Cause
- `__udivdi3` and `__umoddi3` relied on a 64/32 helper that was incorrect for
  large 64-bit dividends.
- `__umoddi3` used `n - q*d` and was sensitive to errors in the division path.

## Investigation

### How the Issue Was Isolated
1. Ran `test_64bit_divide` and recorded failure code.
2. Created minimal repros:
   - `10000000000 / 100000` (udiv)
   - `10000000007 % 10000000000` (umod)
3. Verified incorrect results in the emulator.

## Solution

### Changes Made

- **File**: `m65832-stdlib/compiler-rt/compiler_rt.c`
- Implemented a `udivmod64` helper that computes both quotient and remainder
  using 32-bit operations on `(high, low)` pairs.
- Replaced `__udivdi3` and `__umoddi3` to use `udivmod64` directly.
- Added a safe 64/32 path and a high-word fast path for near-equal operands.

### Why This Fix Works
- The new path avoids reliance on incorrect 64/32 division logic.
- It returns remainder directly from the long-division loop, avoiding
  `n - q*d` overflow sensitivity.

## Testing

```
PASS: debug_udiv64_r2 (A=000186A0, 0s)
PASS: debug_umod64_r8 (A=00000007, 0s)
PASS: test_64bit_divide (A=00000000, 0s)
```

Full core suite:

```
Results: 156 passed, 0 failed, 0 skipped
```

## Related Issues

- Picolibc failures that depended on 64-bit division/modulo should now clear.

