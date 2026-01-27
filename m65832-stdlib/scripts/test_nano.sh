#!/bin/bash
# test_nano.sh - Test the nano libc on the emulator
#
# Runs a series of tests to validate the nano libc works correctly.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
STDLIB_DIR="$(dirname "$SCRIPT_DIR")"
PROJECTS_DIR="$(dirname "$(dirname "$STDLIB_DIR")")"

# Paths
SYSROOT="$PROJECTS_DIR/m65832-nano-sysroot"
LLVM_BUILD="$PROJECTS_DIR/llvm-m65832/build"
EMU="$PROJECTS_DIR/M65832/emu/m65832emu"
TEST_DIR="$STDLIB_DIR/test"
BUILD_DIR="$STDLIB_DIR/build/test"

# Tools
CLANG="$LLVM_BUILD/bin/clang"
LLD="$LLVM_BUILD/bin/ld.lld"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

PASSED=0
FAILED=0

echo "=========================================="
echo "M65832 Nano Libc Tests"
echo "=========================================="
echo ""

# Check prerequisites
if [ ! -f "$SYSROOT/lib/libc.a" ]; then
    echo "ERROR: Nano libc not built. Run ./build_nano.sh first"
    exit 1
fi

mkdir -p "$BUILD_DIR"

# Function to run a test
run_test() {
    local name="$1"
    local src="$2"
    local expected="${3:-0}"
    
    local obj="$BUILD_DIR/${name}.o"
    local elf="$BUILD_DIR/${name}.elf"
    
    # Compile
    "$CLANG" -target m65832-elf -O2 -ffreestanding -nostdlib \
        -I"$SYSROOT/include" \
        -c "$src" -o "$obj" 2>/dev/null
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL${NC}: $name (compile error)"
        FAILED=$((FAILED + 1))
        return
    fi
    
    # Link
    "$LLD" -T "$SYSROOT/lib/m65832.ld" \
        "$SYSROOT/lib/crt0.o" "$SYSROOT/lib/init.o" \
        "$obj" \
        -L"$SYSROOT/lib" -lc -lplatform \
        -o "$elf" 2>/dev/null
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL${NC}: $name (link error)"
        FAILED=$((FAILED + 1))
        return
    fi
    
    # Run
    "$EMU" "$elf" -n 500000 >/dev/null 2>&1
    local exit_code=$?
    
    if [ "$exit_code" = "$expected" ]; then
        echo -e "${GREEN}PASS${NC}: $name"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}FAIL${NC}: $name (expected $expected, got $exit_code)"
        FAILED=$((FAILED + 1))
    fi
}

# Run tests
echo "--- String Tests ---"
run_test "test_simple" "$TEST_DIR/test_simple.c" 0
run_test "test_string_basic" "$TEST_DIR/test_string_basic.c" 0

echo ""
echo "--- Ctype Tests ---"
run_test "test_ctype_min" "$TEST_DIR/test_ctype_min.c" 0
run_test "test_ctype_step" "$TEST_DIR/test_ctype_step.c" 0
run_test "test_ctype_basic" "$TEST_DIR/test_ctype_basic.c" 0

echo ""
echo "--- Stdlib Tests ---"
run_test "test_stdlib_basic" "$TEST_DIR/test_stdlib_basic.c" 0

echo ""
echo "=========================================="
echo -e "Results: ${GREEN}$PASSED passed${NC}, ${RED}$FAILED failed${NC}"
echo "=========================================="

exit $FAILED
