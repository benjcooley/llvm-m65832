# M65832 Varargs ABI — Single Source of Truth

**Do not change varargs behavior without running the regression guard first.**

## ONE TRUE ABI: 4-Byte Stack Slot Alignment

All variadic arguments on the stack use **4-byte alignment**. This is non-negotiable; 8-byte alignment breaks picolibc (printf, sprintf, etc.).

| Component | Rule | Location |
|-----------|------|----------|
| **Data layout** | `i64:32:64` and `f64:32:64` (ABI align 4 bytes) | `M65832TargetMachine.cpp` |
| **Stack slots** | `CCAssignToStack<8, 4>` for i64/f64 | `M65832CallingConv.td` |
| **Min stack align** | `setMinStackArgumentAlignment(Align(4))` | `M65832ISelLowering.cpp` |
| **LowerCall varargs** | `alignTo(VarArgOffset, 4u)` for all types | `M65832ISelLowering.cpp` LowerCall |
| **va_arg expansion** | Uses `DL.getABITypeAlign()` → 4 for double; no extra alignment when MA ≤ 4 | `expandVAArg` (SelectionDAG.cpp) |
| **FirstVarArg** | `CCInfo.getStackSize() + StackArgBase` (12) | `LowerFormalArguments` |

## Touchpoints (Change Only With Caution)

1. **M65832TargetMachine.cpp** — `M65832DataLayout` (`f64:32:64` etc.)
2. **M65832CallingConv.td** — `CCAssignToStack<8, 4>`
3. **M65832ISelLowering.cpp** — `setMinStackArgumentAlignment`, varargs loop, LowerVASTART
4. **Clang/LLVM default** — `EmitVAArgInstr` + `expandVAArg` use DataLayout ABI alignment

## Regression Guard — Must Not Break

Before committing any varargs-related change, run:

```bash
# 1. m65832 c_tests (includes test_varargs, test_varargs_float)
cd m65832-stdlib/c_tests && bash run_tests.sh

# 2. picolibc gtest (includes printf/sprintf/scanf)
cd m65832-stdlib && python3 run_picolibc_gtest.py

# 3. Varargs trio (from test-suite-m65832 root)
# TEST_SUITE_DIR=/path/to/test-suite-m65832 scripts/varargs_regression_guard.sh
build-fast/bin/llvm-lit -sv -j1 \
  SingleSource/Regression/C/Regression-C-callargs.test \
  SingleSource/UnitTests/2003-05-07-VarArgs.test \
  SingleSource/UnitTests/2003-08-11-VaListArg.test
```

If any of these fail after a va_args change, **revert or fix before pushing**.

## Verified Working

- `c_tests/test_varargs_float.c` — printf with 4 doubles passes at O0, O1, O2, O3.
- Same code as test-suite `2006-12-01-float_varg`. Backend va_args for f64 is correct.

## Known Test-Suite Failure (Likely Config, Not ABI)

- `2006-12-01-float_varg` — fails in test-suite run but identical `test_varargs_float` passes in c_tests. Likely test-suite build/config/exe_wrapper difference. Do not change ABI to fix.

## History

- 8-byte vararg alignment was tried and reverted; it breaks picolibc.
- 4-byte is the stable ABI. float_varg needs a targeted fix (if any) that does not alter the ABI.
