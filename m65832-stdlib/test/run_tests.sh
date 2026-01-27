#!/bin/bash
# Comprehensive stdlib test runner
# Compiles test files against the stdlib and runs them on the emulator

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
STDLIB_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$STDLIB_DIR/build"
TEST_BUILD="$BUILD_DIR/test"

# Tools
CLANG="/Users/benjamincooley/projects/llvm-m65832/build/bin/clang-23"
LLD="/Users/benjamincooley/projects/llvm-m65832/build/bin/ld.lld"
EMU="/Users/benjamincooley/projects/M65832/emu/m65832emu"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Counters
PASSED=0
FAILED=0
SKIPPED=0

mkdir -p "$TEST_BUILD"

# Build stdlib if not already built
if [ ! -f "$BUILD_DIR/libc.a" ]; then
    echo "Building stdlib..."
    cd "$STDLIB_DIR" && make
fi

# Compile and link a test
compile_test() {
    local test_src="$1"
    local test_name="$(basename "$test_src" .c)"
    local test_obj="$TEST_BUILD/${test_name}.o"
    local test_elf="$TEST_BUILD/${test_name}.elf"
    
    # Compile
    "$CLANG" -target m65832 -O2 -ffreestanding -nostdlib \
        -I"$STDLIB_DIR/libc/include" \
        -I"/Users/benjamincooley/projects/M65832/emu/platform" \
        -c "$test_src" -o "$test_obj" 2>&1
    
    if [ $? -ne 0 ]; then
        return 1
    fi
    
    # Link
    "$LLD" -T "$STDLIB_DIR/scripts/baremetal/m65832.ld" \
        -o "$test_elf" \
        "$BUILD_DIR/crt0.o" \
        "$BUILD_DIR/init.o" \
        "$test_obj" \
        "$BUILD_DIR/libc.a" \
        "$BUILD_DIR/libplatform.a" 2>&1
    
    if [ $? -ne 0 ]; then
        return 1
    fi
    
    echo "$test_elf"
}

# Run a test
run_test() {
    local test_src="$1"
    local expected="$2"
    local test_name="$(basename "$test_src" .c)"
    
    # Compile
    local compile_output
    compile_output=$(compile_test "$test_src" 2>&1)
    local compile_status=$?
    
    local elf_file="$TEST_BUILD/${test_name}.elf"
    
    if [ $compile_status -ne 0 ] || [ ! -f "$elf_file" ]; then
        echo -e "${RED}FAIL${NC}: $test_name (compilation failed)"
        echo "$compile_output"
        FAILED=$((FAILED + 1))
        return 1
    fi
    
    # Run on emulator
    local output
    output=$("$EMU" "$elf_file" 2>&1)
    local exit_code=$?
    
    # Check result
    if [ -n "$expected" ]; then
        # Expected exit code provided
        if [ "$exit_code" = "$expected" ]; then
            echo -e "${GREEN}PASS${NC}: $test_name (exit=$exit_code)"
            PASSED=$((PASSED + 1))
        else
            echo -e "${RED}FAIL${NC}: $test_name (expected exit=$expected, got exit=$exit_code)"
            FAILED=$((FAILED + 1))
        fi
    else
        # No expected value - just check it ran (exit 0)
        if [ "$exit_code" = "0" ]; then
            echo -e "${GREEN}PASS${NC}: $test_name"
            PASSED=$((PASSED + 1))
        else
            echo -e "${RED}FAIL${NC}: $test_name (exit=$exit_code)"
            FAILED=$((FAILED + 1))
        fi
    fi
}

echo "=========================================="
echo "M65832 Stdlib Test Suite"
echo "=========================================="
echo ""

# Run all tests in test directory
if [ "$1" != "" ]; then
    # Run specific test
    run_test "$1" "$2"
else
    # Run all test_*.c files
    for test_file in "$SCRIPT_DIR"/test_*.c; do
        if [ -f "$test_file" ]; then
            run_test "$test_file" "0"
        fi
    done
fi

echo ""
echo "=========================================="
echo -e "Results: ${GREEN}$PASSED passed${NC}, ${RED}$FAILED failed${NC}, $SKIPPED skipped"
echo "=========================================="

if [ $FAILED -gt 0 ]; then
    exit 1
fi
exit 0
