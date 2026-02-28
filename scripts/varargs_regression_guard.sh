#!/usr/bin/env bash
# Varargs regression guard — run before committing any va_args-related change.
# All checks must pass. See docs/ABI-varargs.md for details.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

FAILED=0

echo "=== M65832 Varargs Regression Guard ==="
echo

# 1. c_tests (includes test_varargs, test_varargs_float)
if [[ -f "$ROOT/m65832-stdlib/c_tests/run_tests.sh" ]]; then
  echo "[1/3] m65832 c_tests..."
  if (cd "$ROOT/m65832-stdlib/c_tests" && bash run_tests.sh); then
    echo "      PASS"
  else
    echo "      FAIL"
    FAILED=1
  fi
else
  echo "[1/3] m65832 c_tests — SKIP (c_tests/run_tests.sh not found)"
fi
echo

# 2. picolibc gtest
if [[ -f "$ROOT/m65832-stdlib/run_picolibc_gtest.py" ]]; then
  echo "[2/3] picolibc gtest..."
  if (cd "$ROOT/m65832-stdlib" && python3 run_picolibc_gtest.py 2>/dev/null); then
    echo "      PASS"
  else
    echo "      FAIL (or run manually: cd m65832-stdlib && python3 run_picolibc_gtest.py)"
    FAILED=1
  fi
else
  echo "[2/3] picolibc gtest — SKIP (run_picolibc_gtest.py not found)"
fi
echo

# 3. Varargs trio (test-suite)
LIT="${LIT_BIN:-$ROOT/build-fast/bin/llvm-lit}"
TS_DIR="${TEST_SUITE_DIR:-}"
if [[ -n "$TS_DIR" && -d "$TS_DIR" && -x "$LIT" ]]; then
  echo "[3/3] Varargs trio (test-suite)..."
  if "$LIT" -sv -j1 "$TS_DIR/SingleSource/Regression/C/Regression-C-callargs.test" \
       "$TS_DIR/SingleSource/UnitTests/2003-05-07-VarArgs.test" \
       "$TS_DIR/SingleSource/UnitTests/2003-08-11-VaListArg.test" 2>/dev/null; then
    echo "      PASS"
  else
    echo "      FAIL"
    FAILED=1
  fi
else
  echo "[3/3] Varargs trio — SKIP (set TEST_SUITE_DIR and ensure build-fast/bin/llvm-lit exists)"
  echo "      Example: TEST_SUITE_DIR=/path/to/test-suite-m65832 $0"
fi
echo

if [[ $FAILED -eq 0 ]]; then
  echo "=== All varargs regression checks passed ==="
  exit 0
else
  echo "=== Some checks failed — do not commit va_args changes until fixed ==="
  exit 1
fi
