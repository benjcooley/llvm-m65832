# Debug progress (M65832 + picolibc)

## What I am seeing now
- The remaining picolibc failures are consistent with a backend bug in **stack-local array indexing**.
- A minimal repro shows the issue:
  ```c
  char buf[] = "abcdefgh";
  return (unsigned char)buf[2];
  ```
  This returns `0` instead of `'c'`.
- The generated assembly for the direct `buf[2]` load contains:
  ```
  LD.B R0, @<noreg>, Y
  ```
  which effectively reads from address `0x00000002` (base is missing).  
  This explains failures in the string/memmove suite and other libc tests
  that read stack-local arrays without first taking a pointer.
- If I rewrite to use a pointer first:
  ```c
  char *p = buf;
  return (unsigned char)p[2];
  ```
  it works. So pointer-based addressing is correct, **direct stack indexing is not**.

## What I am fixing
- The bug is in **frame index selection for memory operands**.
- When the address is a `FrameIndex` (stack slot), the DAG selector is currently
  replacing it with a `LEA_FI` value node in all cases.
- That replacement is safe for uses like `&buf`, but **wrong for memory ops**
  (`load`/`store`), because the frame index needs to survive into the
  memory operand so `eliminateFrameIndex` can compute the real base+offset.

## Fix approach
1. **Change instruction selection for `ISD::FrameIndex`:**
   - Only replace a `FrameIndex` with `LEA_FI` when it is used as a value
     (e.g., `&buf`, pointer arithmetic).
   - If the `FrameIndex` feeds a `load`/`store` (or an `add` feeding a
     `load`/`store`), skip replacement so it stays as a frame index for
     `selectAddr` and `eliminateFrameIndex`.
2. Keep `selectAddr` unchanged so:
   - `FrameIndex` → `(Base=FI, Offset=0)`
   - `ADD(FrameIndex, imm)` → `(Base=FrameIndex, Offset=imm)`
3. Rebuild clang and re-run the minimal repro.
   - Expect the direct `buf[2]` read to emit a valid base register instead
     of `@<noreg>`.
4. Re-run the failing picolibc tests (especially string/memmove suites).

## Current status
- Repro and diagnosis are stable and repeatable.
- Fix is focused on `M65832ISelDAGToDAG.cpp`:
  - `Select(ISD::FrameIndex)` should become use-aware.
  - Goal: ensure stack-local loads/stores preserve frame indices.

