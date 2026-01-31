# Fix: [Short description of the issue]

**Date**: YYYY-MM-DD  
**Author**: [Your name or "AI Agent"]  
**Severity**: [Critical | High | Medium | Low]  
**Component**: [Backend | Instruction Selection | Register Allocation | Lowering | Library | etc.]

## Problem

### Symptoms
- Which tests were failing?
- What was the error message or behavior?
- How did you discover this issue?

Example:
```
Tests failing: libc_testsuite.strtol, baremetal.stdlib_atoi_neg
Error: LLVM ERROR: Cannot select: t6: i32 = mul i32, i32
Symptom: Compilation fails when code contains integer multiplication
```

### Root Cause
- What was the underlying problem?
- Which component had the bug?
- Why did it fail?

Example:
```
The M65832 backend was missing TableGen patterns for i32 multiplication.
While the MUL32rr instruction was defined, there was no pattern matching
(mul i32, i32) to this instruction, causing instruction selection to fail.
```

## Investigation

### How the Issue Was Isolated
- What steps did you take to isolate the problem?
- Which debug tools did you use?
- What was the minimal failing test case?

Example:
```
1. Started with failing test: libc_testsuite/strtol.c
2. Bisected test to find strtol calls multiplication internally
3. Created minimal test:
   ```c
   int main() { return 10 * 5; }
   ```
4. Compiled with -mllvm -debug-only=isel
5. Saw error: "Cannot select: mul i32"
6. Checked M65832InstrInfo.td - MUL32rr defined but no pattern
```

### Debug Output
- Include relevant LLVM debug output
- Show the SelectionDAG or IR that failed

Example:
```
LLVM IR:
  %3 = mul i32 %1, %2

Initial selection DAG:
  t6: i32 = mul i32 t2, i32 t4
  
LLVM ERROR: Cannot select: t6: i32 = mul t2, t4

Checked M65832InstrInfo.td:
  def MUL32rr : M65832Inst<...>; // Instruction defined
  # But no pattern matching (mul i32, i32)!
```

## Solution

### Changes Made
- List all files modified
- Describe what you changed and why
- Show code diffs for key changes

Example:
```
Files modified:
- llvm/lib/Target/M65832/M65832InstrInfo.td

Added pattern to match i32 multiplication:

```diff
+// Pattern to match integer multiplication
+def : Pat<(mul GR32:$src1, GR32:$src2), 
+          (MUL32rr GR32:$src1, GR32:$src2)>;
```

This tells LLVM to use the MUL32rr instruction when it sees
a multiplication of two 32-bit general registers.
```

### Why This Fix Works
- Explain the reasoning behind your solution
- Describe any trade-offs or alternative approaches considered

Example:
```
The pattern `(mul GR32:$src1, GR32:$src2)` matches the LLVM IR operation
for i32 multiplication. When the instruction selector sees this pattern,
it now knows to generate the MUL32rr machine instruction.

Alternative considered: Could have implemented custom lowering in
M65832ISelLowering.cpp, but a simple pattern is more maintainable.
```

### Testing
- List tests that now pass
- Show test output
- Describe any new tests you created

Example:
```
Tests now passing:
- libc_testsuite.strtol ✓
- baremetal.stdlib_atoi_neg ✓

Created comprehensive test:
- c_tests/baremetal/picolibc/test_mul_comprehensive.c
  - 15 subtests covering: basic mul, zero, negative, large values, mixed types
  - All 15 subtests passing

Test output:
[==========] Running 15 tests from arithmetic
[  PASSED  ] 15 tests.

Full regression test results:
- Baremetal: 27/27 passing (no regressions)
- libc_testsuite: 6/10 passing (was 5/10, +1)
```

## Related Issues

- List any related bugs or missing features
- Link to similar fixes or issues
- Note any follow-up work needed

Example:
```
Related issues:
- Division (DIV32rr) also needs patterns - TODO
- 64-bit multiplication not yet supported - future work
- Optimization: mul by power-of-2 constant could use shifts - optimization

Follow-up work:
- Add patterns for div, mod operations
- Create comprehensive arithmetic test suite for all ops
- Document all supported arithmetic operations
```

## References

- Link to LLVM documentation
- Cite any external resources used
- Note any discussions or design decisions

Example:
```
References:
- LLVM TableGen documentation: https://llvm.org/docs/TableGen/
- Pattern matching: https://llvm.org/docs/TableGen/ProgRef.html#patterns
- M65832 ISA specification: docs/M65832-ISA.md
- Similar implementation in RISC-V: llvm/lib/Target/RISCV/RISCVInstrInfo.td:567
```

## Code References

### Before (Broken)
```tablegen
// M65832InstrInfo.td - line 234
def MUL32rr : M65832Inst<(outs GR32:$dst), (ins GR32:$src1, GR32:$src2),
                         "mul $dst, $src1, $src2",
                         []>;  // Empty pattern list - no matching!
```

### After (Fixed)
```tablegen
// M65832InstrInfo.td - line 234
def MUL32rr : M65832Inst<(outs GR32:$dst), (ins GR32:$src1, GR32:$src2),
                         "mul $dst, $src1, $src2",
                         [(set GR32:$dst, (mul GR32:$src1, GR32:$src2))]>;
                         ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                         Added pattern to match (mul i32, i32)
```

---

## Checklist

Before marking this fix complete:

- [ ] Created minimal test case demonstrating the issue
- [ ] Identified root cause with LLVM debug output
- [ ] Implemented and tested the fix
- [ ] Created comprehensive tests for this functionality
- [ ] Ran regression tests (baremetal suite at minimum)
- [ ] Documented the fix in this file
- [ ] Updated docs/fixes/README.md index
- [ ] Committed changes with descriptive message
- [ ] No new test failures introduced
