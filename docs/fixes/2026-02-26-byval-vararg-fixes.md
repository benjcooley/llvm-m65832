# Byval struct fix (2026-02-26)

## Problem

**StructureArgs, StructureArgsSimple, byval-alignment**: Output empty or wrong when passing structs by value (e.g. `void print(struct vec2 S, struct vec2 T)`).

## Root cause

### Byval
- **Caller**: M65832 LowerCall was storing the struct *pointer* to the stack instead of copying the struct contents. CCPassByVal expects the caller to copy the pointee.
- **Callee**: LowerFormalArguments was loading the first 4 bytes and passing that, but the parameter is a pointer — the callee needs the address of the stack slot (FrameIndex), not the loaded value.

## Fix

### 1. LowerCall (M65832ISelLowering.cpp)
- When `Outs[ValNo].Flags.isByVal()` and `VA.isMemLoc()`, copy the struct from the pointer to the stack in 4-byte chunks instead of storing the pointer.
- Use `OutVals[ValNo]` (correct argument index via `VA.getValNo()`).

### 2. LowerFormalArguments
- When `Ins[i].Flags.isByVal()`, create a FixedObject with `getByValSize()` and pass `DAG.getFrameIndex(FI, MVT::i32)` as the value (pointer to stack slot).

## Tests fixed
- 2002-10-12-StructureArgs
- 2002-10-12-StructureArgsSimple  
- byval-alignment

## Note on vararg alignment
M65832 ABI uses 4-byte alignment for 64-bit arguments (f64/i64) in varargs. Do not change to 8-byte — that breaks picolibc and other libs.
