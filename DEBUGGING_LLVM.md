# LLVM debugging notes (M65832)

This document captures the tools and command lines used to debug the picolibc
memmove/memcpy failure caused by incorrect stack-local addressing.

## Minimal repro

We use a tiny C file that forces a stack-local array and a read from it:

- `memmove_repro.c`

## Build prerequisites

This repo uses a `build-fast` tree. Ensure `clang` and `llc` exist:

- `ninja -C /Users/benjamincooley/projects/llvm-m65832/build-fast clang`
- `ninja -C /Users/benjamincooley/projects/llvm-m65832/build-fast llc`

## Key diagnostic commands

### 1) Default (libcall) path

This is the baseline where memcpy/memmove are libcalls:

- Emit LLVM IR:
  - `/Users/benjamincooley/projects/llvm-m65832/build-fast/bin/clang -target m65832-elf -O1 -S -emit-llvm memmove_repro.c -o memmove_repro.ll`
- Emit assembly:
  - `/Users/benjamincooley/projects/llvm-m65832/build-fast/bin/clang -target m65832-elf -O1 -S memmove_repro.c -o memmove_repro.s`

Result: the stack-local load uses a valid base (no `@<noreg>`).

### 2) Force inline memcpy/memmove

This reproduces the failure:

- `/Users/benjamincooley/projects/llvm-m65832/build-fast/bin/clang -target m65832-elf -O1 -S memmove_repro.c -o memmove_repro_inline.s -mllvm -max-store-memcpy=8 -mllvm -max-store-memmove=8`

Result: the direct stack load becomes `LD.B R0, @<noreg>, Y` in assembly.

### 3) Stop after ISel

This shows the broken address *already in ISel* (before regalloc/constraints):

- `/Users/benjamincooley/projects/llvm-m65832/build-fast/bin/llc -mtriple=m65832-elf -O1 -stop-after=finalize-isel -o memmove_repro.isel.mir memmove_repro.ll`
- `/Users/benjamincooley/projects/llvm-m65832/build-fast/bin/llc -mtriple=m65832-elf -O1 -max-store-memcpy=8 -max-store-memmove=8 -stop-after=finalize-isel -o memmove_repro_inline.isel.mir memmove_repro.ll`

Result: `memmove_repro_inline.isel.mir` contains a `LOAD8` with `$noreg` base,
so the base is lost during ISel, not later.

### 4) Stop after post-RA pseudo expansion

This confirms post-RA behavior:

- `/Users/benjamincooley/projects/llvm-m65832/build-fast/bin/llc -mtriple=m65832-elf -O1 -stop-after=postrapseudos -o memmove_repro.postra.mir memmove_repro.ll`
- `/Users/benjamincooley/projects/llvm-m65832/build-fast/bin/llc -mtriple=m65832-elf -O1 -max-store-memcpy=8 -max-store-memmove=8 -stop-after=postrapseudos -o memmove_repro_inline.postra.mir memmove_repro.ll`

Result: the inline case still reflects the broken base from ISel.

### 5) Pass pipeline (for bisecting)

This prints the pass pipeline to identify the exact stage:

- `/Users/benjamincooley/projects/llvm-m65832/build-fast/bin/llc -mtriple=m65832-elf -O1 -debug-pass=Structure -o memmove_repro.s memmove_repro.ll`

The pipeline shows `finalize-isel` as the step where the MIR is first selected.

## Core compiler regression tests (non-picolibc)

These tests live in the emulator repo and exercise **core compiler**
functionality only (no libc/picolibc dependency).

### Run all core tests

```
bash /Users/benjamincooley/projects/m65832/emu/c_tests/run_core_tests.sh
```

### Example result

```
Results: 151 passed, 0 failed, 0 skipped
```

### Notes

- Tests are located in `/Users/benjamincooley/projects/m65832/emu/c_tests/baremetal/core/`.
- The harness uses `run_c_test.sh` and the emulator, and expects a working
  compiler + compiler runtime (startup/soft-float/soft-int), not picolibc.

## Picolibc tests without Meson (Python runner)

The non-Meson picolibc runner lives under the emulator tree and builds tests
with the sysroot linker script (correct load address):

- Script: `m65832/emu/c_tests/run_picolibc_gtest.py`
- Linker script: `m65832-sysroot/lib/m65832.ld`
- CRT: `m65832-sysroot/lib/crt0.o`

Usage:

```
python /Users/benjamincooley/projects/m65832/emu/c_tests/run_picolibc_gtest.py --list
python /Users/benjamincooley/projects/m65832/emu/c_tests/run_picolibc_gtest.py --filter=memmove
```

This is the preferred path when Meson builds are not being used.

## Identifying miscompiles (LLVM tools)

Miscompiles often show up as: program hangs, wrong return value, or only fails
at higher optimization levels. The workflow below isolates where the bug lives.

### 1) Prove it’s optimization-related

- Compare `-O0` vs `-O2` behavior.
- If `-O0` works and `-O2` fails, the bug is in the optimizer or backend.

### 2) Lock down the IR

Generate IR once, then compile it independently:

- Emit IR:
  - `clang -target m65832-elf -O2 -S -emit-llvm repro.c -o repro_O2.ll`
- Compile IR with llc:
  - `llc -mtriple=m65832-elf -O2 repro_O2.ll -o repro_O2.s`

Example:

- `clang -target m65832-elf -O2 -S repro.c -o repro.s` (hangs)
- `clang -target m65832-elf -O2 -S -emit-llvm repro.c -o repro_O2.ll` (works)
- `llc -mtriple=m65832-elf -O2 repro_O2.ll -o repro_O2.s` (works)

If `clang -S` hangs but `llc` succeeds on the same IR, the issue is in the
clang codegen pipeline or target-specific passes invoked by the driver.

### 3) Use MIR snapshots to localize

- `llc -mtriple=m65832-elf -O2 -stop-after=finalize-isel -o out.isel.mir repro_O2.ll`
- `llc -mtriple=m65832-elf -O2 -stop-after=postrapseudos -o out.postra.mir repro_O2.ll`

Example: bad address is present in ISel MIR:

- `out.isel.mir` shows `LOAD8` with `$noreg` base
- `out.postra.mir` shows the same base, so the bug is already in ISel

Compare MIR to locate the first point the miscompile appears.

### 4) Pass bisect (opt-bisect)

Use opt-bisect to identify the transform that first triggers failure:

- `clang -target m65832-elf -O2 -S repro.c -o repro.s -mllvm -opt-bisect-limit=N`

Example:

- `-opt-bisect-limit=20` works
- `-opt-bisect-limit=30` hangs

Start with small `N` and increase until the failure appears.

### 5) ISel input/output checks

When the bug smells like instruction selection:

- `llc -mtriple=m65832-elf -O2 --print-isel-input --print-after-isel repro_O2.ll`

Example red flags:

- `@<noreg>` in assembly
- `LOAD8 %addr, 0` where `%addr` is a FrameIndex, not a GPR
- DAG contains `OR(FrameIndex, const)` without a matching selectAddr case

Look for malformed addresses (`@<noreg>`, missing base) or illegal operands.

### 6) Minimal reproducer

Reduce the C source to the smallest pattern that still triggers the bug.
Keep the minimal C file and the resulting IR so future debugging is faster.

## Notes on unavailable tooling

The `llc` in this tree does not accept `-dot-isel-dags` or `-print-passes`.
Use `-debug-pass=Structure` and the `-stop-after` MIR snapshots instead.

## Summary of findings

- The bad `@<noreg>` addressing is created during instruction selection.
- It appears only when memcpy/memmove are inlined into stores/loads.
- Forcing memcpy/memmove to libcalls avoids the bad path.
 - Inline path shows the load pointer as `or(FrameIndex, Constant)`; `selectAddr`
   did not handle `ISD::OR`, so the base was dropped and the load used `$noreg`.

### Store-value drop in ISel (core tests)

Symptom: a simple `result = result * 10 + 1` sequence miscompiles to
`result = result * 10`, causing `test_64bit_basic` to return `6`.

Where to look:

- `llc -mtriple=m65832-elf -O0 -stop-after=finalize-isel -o out.isel.mir repro.ll`
- If you see `STORE32 killed $noreg` or a store with `$noreg` value, the store
  value was dropped during ISel.

Minimal repro (no libc):

- `debug_strtol_like.c` (manual `result = result * 10 + digit` loop)
- The assembly shows `LD R0,#$01` followed by `LDA R38` instead of using the
  immediate value, which confirms the dropped store value.

## Fix that resolved the repro

Handle `ISD::OR` in `selectAddr` when it is effectively `FrameIndex | constant`
for stack offsets:

- File: `llvm/lib/Target/M65832/M65832ISelDAGToDAG.cpp`
- Change: in `selectAddr`, treat `ISD::OR` with `FrameIndex` + constant as
  `(Base=TargetFrameIndex, Offset=constant)` just like `ADD`.

After this change, the inline memcpy path emits a valid stack-relative byte
load (no `@<noreg>`).

## Quick checklist (triage flow)

Use this as a fast path when a new bug appears:

1. **Reproduce at `-O0` vs `-O2`** to confirm optimizer involvement.
2. **Freeze IR** with `-emit-llvm` and compile with `llc`.
3. **Check ISel output** (`--print-isel-input --print-after-isel`).
4. **Snapshot MIR** at `finalize-isel` and `postrapseudos`.
5. **Bisect passes** with `-opt-bisect-limit`.
6. **Reduce repro** to minimal C / IR.
7. **Add regression test** once fixed.

## What to capture for a bug report

Always save these artifacts:

- Minimal C repro (`.c`)
- IR at failing optimization level (`.ll`)
- ISel MIR (`-stop-after=finalize-isel`)
- Post-RA MIR (`-stop-after=postrapseudos`)
- Assembly output (`.s`)
- Exact compile command

This makes it trivial to re-run the investigation later or for another engineer.

## Target comparison workflow

If you suspect target-specific lowering issues:

1. Emit IR for a known-good 32-bit target:
   - `clang -target i686-unknown-linux-gnu -O2 -S -emit-llvm repro.c -o repro_i686.ll`
   - `clang -target armv7-none-eabi -O2 -S -emit-llvm repro.c -o repro_armv7.ll`
2. Compare against M65832 IR to see how varargs, pointer arithmetic, or
   addressing are represented.
3. Try compiling those IRs with `llc -mtriple=m65832-elf` to see if the issue
   is specific to M65832 lowering.

## Common root causes in ISel

When the selector spins or emits `@<noreg>`, check for:

- **FrameIndex in base+offset forms** (must be `TargetFrameIndex`)
- **`OR` used as pointer arithmetic** (treat as `ADD` or handle directly)
- **Address nodes used as values** (FI vs LEA_FI misuse)
- **Missing addressing patterns** (`ADDRri` doesn’t match DAG shape)

## Regalloc vs ISel vs Lowering

Use this to place the bug quickly:

- **Lowering bug**: IR is malformed or violates target constraints.
- **ISel bug**: MIR after `finalize-isel` already contains broken operands.
- **Regalloc/PEI bug**: MIR is fine before regalloc, breaks after `postrapseudos`.

## Emulator-focused checks

If assembly looks valid but the program still misbehaves:

- Run with smaller cycle limit to catch infinite loops quickly.
- Compare emulator output for registers and memory after key instructions.
- Reduce the test to return a single byte or small integer for fast diagnosis.

## ELF load failures in picolibc string tests

If Meson reports `Failed to load ELF ... test-memmove_s` (or similar), first
verify the ELF entry point and confirm it is legal for M65832:

### 1) Inspect entry point quickly

```
python - <<'PY'
import struct
path = "/Users/benjamincooley/projects/picolibc-build-m65832-tests/test/test-string/test-memmove_s"
with open(path, "rb") as f:
    data = f.read(0x34)
print(f"entry=0x{struct.unpack_from('<I', data, 0x18)[0]:08X}")
PY
```

If this prints `entry=0x00000000`, that is **illegal** for M65832 and the
emulator should reject it. In this case, the issue is likely in the linker
script or build flags, not in the emulator or LLVM ISel.

### 2) Confirm `_start` placement

```
python - <<'PY'
import subprocess
path = "/Users/benjamincooley/projects/picolibc-build-m65832-tests/test/test-string/test-memmove_s"
out = subprocess.check_output(["nm", path], text=True)
for line in out.splitlines():
    if " _start" in line:
        print(line)
PY
```

If `_start` resolves to `0x00000000`, investigate why the build is placing
`.text` at zero and switch to the known-good test path that was working
previously.
