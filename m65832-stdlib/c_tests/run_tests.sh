#!/bin/bash
# Standalone C tests runner for M65832
# These tests don't depend on libc - just compiler-rt for 64-bit ops

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# Tools
CLANG="/Users/benjamincooley/projects/llvm-m65832/build-fast/bin/clang"
LLD="/Users/benjamincooley/projects/llvm-m65832/build/bin/ld.lld"
EMU="/Users/benjamincooley/projects/m65832/emu/m65832emu"

SYSROOT="/Users/benjamincooley/projects/m65832-sysroot"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASSED=0
FAILED=0

mkdir -p "$BUILD_DIR"

run_test() {
    local test_src="$1"
    local test_name="$(basename "$test_src" .c)"
    local test_obj="$BUILD_DIR/${test_name}.o"
    local test_elf="$BUILD_DIR/${test_name}.elf"
    
    # Compile - no libc, just compiler-rt for 64-bit ops
    "$CLANG" -target m65832-elf -O2 -ffreestanding \
        -I"$SYSROOT/include" \
        -c "$test_src" -o "$test_obj" 2>&1
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL${NC}: $test_name (compilation failed)"
        FAILED=$((FAILED + 1))
        return 1
    fi
    
    # Link with crt0 and compiler-rt only
    "$LLD" -T "$SYSROOT/lib/m65832.ld" \
        -o "$test_elf" \
        "$SYSROOT/lib/crt0.o" \
        "$test_obj" \
        -L"$SYSROOT/lib" -lcompiler_rt 2>&1
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL${NC}: $test_name (link failed)"
        FAILED=$((FAILED + 1))
        return 1
    fi
    
    # Run with generous cycle limit
    local output
    output=$("$EMU" -c 10000000 -s --stop-on-brk "$test_elf" 2>&1)
    local exit_code
    exit_code=$(echo "$output" | grep "EXIT:" | sed 's/.*EXIT: \([0-9A-Fa-f]*\).*/\1/' | tail -1)
    if [ -z "$exit_code" ]; then
        exit_code=$(echo "$output" | grep "  A:" | sed 's/.*A: \([0-9A-Fa-f]*\).*/\1/' | tail -1)
    fi
    exit_code=$((16#${exit_code:-FF}))
    
    if [ "$exit_code" = "0" ]; then
        echo -e "${GREEN}PASS${NC}: $test_name"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}FAIL${NC}: $test_name (exit=$exit_code)"
        FAILED=$((FAILED + 1))
    fi
}

echo "=========================================="
echo "M65832 Standalone C Tests"
echo "=========================================="
echo ""

# Run specific test or all tests
if [ "$1" != "" ]; then
    run_test "$1"
else
    for test_file in "$SCRIPT_DIR"/test_*.c; do
        if [ -f "$test_file" ]; then
            run_test "$test_file"
        fi
    done
fi

echo ""
echo "=========================================="
echo -e "Results: ${GREEN}$PASSED passed${NC}, ${RED}$FAILED failed${NC}"
echo "=========================================="

if [ $FAILED -gt 0 ]; then
    exit 1
fi
exit 0
