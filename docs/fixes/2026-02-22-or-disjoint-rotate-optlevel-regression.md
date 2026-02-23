# Fix Note: disjoint OR selection + rotate legality

**Date**: 2026-02-22  
**Component**: `llvm/lib/Target/M65832`  
**Status**: Implemented and validated

## What changed

1. `M65832ISelDAGToDAG.cpp`
   - Kept the special-case lowering for `OR disjoint(FrameIndex, const)` to `LEA_FI`.
   - Removed forced lowering of all other disjoint `OR` nodes to `ADD`.
   - Non-FrameIndex disjoint `OR` now falls through to normal `ISD::OR` selection.

2. `M65832ISelLowering.cpp`
   - Finalized rotate legality with native rotate enabled at all optimization levels:
     - `ROTL`/`ROTR` are `Legal` (native rotate enabled at `-O0/-O1/-O2/-O3`).
   - Retained comments documenting the efficiency rationale for native rotate.

## Why

- The previous forced disjoint-OR-to-ADD path could produce bad selection behavior and regressions in real code paths.
- Native rotate is generally more efficient than expanded shift/or sequences, and final validation showed it is stable at all tested optimization levels.

## Validation summary

- Targeted regressions pass at all optimization levels (`-O0/-O1/-O2/-O3`):
  - `regress_select_cmov`
  - `regress_rol_encoding`
  - `regress_varargs_string`
- Full `c_tests` run passed.
- Full `run_picolibc_gtest.py --no-rebuild` run passed with no failures (expected skips only).
