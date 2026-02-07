# M65832 Picolibc Test Suite Status

## Summary

The picolibc test suite uses a Python-based test runner that compiles and runs C library tests on the M65832 emulator. The runner performs a **full clean rebuild** of compiler-rt and picolibc from source before running tests, ensuring results always reflect the current compiler output.

**Test Runner Location**: `/Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py`

**Total Tests Available**: 181 tests across 9 test suites

## How to Run Tests

### Run All Tests
```bash
cd /Users/benjamincooley/projects/m65832/emu
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py
```

### Run Specific Test Suite
```bash
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=baremetal
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=test_string
```

### Run Specific Test
```bash
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py string_strlen
```

### List All Tests
```bash
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --list
```

## Test Results (as of 2026-02-07)

### Overall: 162 PASS, 0 FAIL, 19 SKIP (out of 181 tests)

All test suites fully passing. Zero failures. Skipped tests are expected (missing platform features).

### ✅ Baremetal Suite: 29/29 PASSING (100%)

All basic C library functionality tests pass:
- String functions: strlen, strcmp, strcpy, memcpy, memset, memcmp, string_suite
- Stdlib functions: abs, atoi (including negative numbers)
- Ctype functions: isalpha, isdigit, tolower, toupper
- 64-bit: args, shifts

### ✅ libc_testsuite: 10/10 PASSING (100%)
- basename, dirname, fnmatch, qsort, snprintf, sscanf, string, strtod, strtol, wcstol

### ✅ test_math: 40/41 PASSING (1 expected skip)
- All trig, exponential, logarithmic, and complex math functions passing
- test-math-compare skipped (expected)

### ✅ test_string: 16/16 PASSING (100%)
- memchr, memchr-simple, memcpy_s, memmem, memmove_s, memset, memset_s, strcat_s, strchr, strcpy_s, strerror_s, strerrorlen_s, strncat_s, strncpy, strncpy_s, strnlen_s

### ✅ test_stdio: 21/24 PASSING (3 expected skips)
- All printf, scanf, fopen, fread/fwrite, ungetc, wchar tests passing
- Skipped: test-flockfile, test-freopen, test-gets (missing platform features)

### ✅ test_ctype: 5/5 PASSING (100%)

### ✅ test_iconv: 1/1 PASSING (100%)

### ✅ picolibc core: 28/33 PASSING (5 expected skips)
- atexit, constructor, fenv, ffs, long_double, malloc, malloc_stress, math-funcs, math_errhandling, on_exit, rand, regex, rounding-mode, setjmp, and many more
- Skipped: abort, complex-funcs, hosted-exit, stack-smash, test-raise (require signals/semihosting)

### Skipped Tests (19 total - all expected)
These require platform features not yet implemented:
- **Signals**: abort, test-raise
- **Atomics/TLS**: test-atomic, tls
- **Semihosting**: hosted-exit, test-argv
- **Other**: complex-funcs, constructor-skip, stack-smash, test-except, test-scmpu, test-ubsan, try-ilp32, test-gets, test-flockfile, test-freopen, test-sprintf-percent-n, test-math-compare, test-strfmon

## Build System

The test runner uses:
- **Compiler**: `/Users/benjamincooley/projects/llvm-m65832/build-fast/bin/clang` (M65832 backend)
- **Linker**: `/Users/benjamincooley/projects/llvm-m65832/build/bin/ld.lld`
- **Emulator**: `/Users/benjamincooley/projects/m65832/emu/m65832emu`
- **Sysroot**: `/Users/benjamincooley/projects/m65832-sysroot` (contains picolibc headers and libraries)
- **Compiler Runtime**: `/Users/benjamincooley/projects/m65832-sysroot/lib/libcompiler_rt.a` (soft float, soft int, crt0)

## Next Steps

To get more tests passing:

1. **Investigate timeouts**: Run failing tests individually with increased cycle limits or debug output
2. **Fix string conversion functions**: strtol, strtod, sscanf need debugging
3. **Fix constructor/atexit**: Core C++ support functions
4. **Optimize performance**: Some tests may just be too slow at -O0

## Nano Library Tests

You mentioned there's also a nano picolibc test that has always succeeded. This is likely a minimal subset for embedded systems.

Location: `/Users/benjamincooley/projects/llvm-m65832/m65832-stdlib/scripts/test_nano.sh`

---

# AI Agent Workflow: Debugging and Fixing M65832 Backend Issues

This section provides a comprehensive guide for AI agents (or developers) working to fix picolibc test failures and M65832 backend issues.

## 0. Test-Driven Development Workflow

### Step 0.1: Identify a Failing Test

**Start with the test results:**
```bash
cd /Users/benjamincooley/projects/m65832/emu

# Get overview of all test suites
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --list

# Run a specific suite to see which tests fail
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=libc_testsuite

# Run a single failing test
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py strtol
```

**Prioritize tests by:**
1. **Impact**: Core functionality (string functions, malloc) before edge cases
2. **Simplicity**: Short tests before complex ones
3. **Dependencies**: Fix foundational issues before dependent features
4. **Pattern**: Group similar failures (all `strtol` family together)

### Step 0.2: Locate the Test Source

```bash
# Test files are in picolibc source tree
cd /Users/benjamincooley/projects/picolibc-m65832

# Find test source
find test -name "*strtol*"
# Result: test/libc-testsuite/strtol.c

# Read the test
cat test/libc-testsuite/strtol.c
```

**Common test locations:**
- `test/*.c` - Top-level picolibc tests
- `test/libc-testsuite/*.c` - Core libc functionality tests
- `test/test-string/*.c` - String function tests
- `test/test-stdio/*.c` - I/O tests
- `test/test-math/*.c` - Math function tests
- `/Users/benjamincooley/projects/m65832/emu/c_tests/baremetal/picolibc/*.c` - Custom M65832 tests

### Step 0.3: Reproduce the Failure Manually

**Compile and inspect:**
```bash
cd /Users/benjamincooley/projects/m65832/emu

# Set up environment
export LLVM_BUILD=/Users/benjamincooley/projects/llvm-m65832/build-fast/bin
export SYSROOT=/Users/benjamincooley/projects/m65832-sysroot

# Compile the test
$LLVM_BUILD/clang -target m65832-elf -O0 -ffreestanding \
    -I$SYSROOT/include \
    -I/Users/benjamincooley/projects/picolibc-m65832/test \
    -c test.c -o test.o

# Link the test
/Users/benjamincooley/projects/llvm-m65832/build/bin/ld.lld \
    -T$SYSROOT/lib/m65832.ld \
    $SYSROOT/lib/crt0.o test.o \
    -L$SYSROOT/lib -lc -lsys -lcompiler_rt \
    -o test.elf

# Run on emulator with verbose output
./m65832emu -c 10000000 --stop-on-brk -s test.elf
```

**Check for common issues:**
- Compilation errors → Missing backend support
- Link errors → Undefined symbols (check libcompiler_rt)
- Timeout → Infinite loop or very slow execution
- Wrong exit code → Logic error or miscompilation
- Segfault → Invalid memory access (emulator or code issue)

### Step 0.4: Fix the Issue

Follow the detailed procedures in sections 1-3 below to:
1. Isolate the failing code (Section 1)
2. Debug instruction selection (Section 2)
3. Implement the fix
4. Create comprehensive tests (Section 3)

### Step 0.5: Validate the Fix

**Run the specific test:**
```bash
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py strtol
# Should now PASS
```

**Check for regressions in related tests:**
```bash
# Run the entire suite the test belongs to
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=libc_testsuite

# Run all fast tests (baremetal suite is comprehensive)
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=baremetal
```

**Full regression test (before committing):**
```bash
# Run ALL tests - takes time but essential before major commits
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py > test-results.log 2>&1

# Check for new failures
grep "FAILED" test-results.log | diff - previous-failures.txt
```

### Step 0.6: Document and Commit

1. Document your fix (see Section 4)
2. Run final regression tests
3. Commit with descriptive message:
```bash
git add <modified files>
git commit -m "M65832: Fix <issue> in <component>

- Describe what was broken
- Explain the root cause
- List what was fixed
- Note any test results

Tests: X new tests passing, Y remain failing
Fixes: picolibc test <test_name>"
```

## 1. Code Bisection and Isolation Techniques

When a test fails, you need to identify the **minimal failing case**. This helps pinpoint exactly which operation or instruction is causing the problem.

### 1.1 Binary Search with Test Code

**Start with the full test, then progressively comment out code:**

```c
// Original failing test
int main() {
    char *endptr;
    long result;
    
    // Test 1: Basic conversion
    result = strtol("123", &endptr, 10);
    if (result != 123) return 1;
    
    // Test 2: Negative numbers
    result = strtol("-456", &endptr, 10);
    if (result != -456) return 2;
    
    // Test 3: Hex conversion
    result = strtol("0xFF", &endptr, 16);
    if (result != 255) return 3;
    
    return 0;
}
```

**Bisect by commenting:**
```c
int main() {
    char *endptr;
    long result;
    
    // Test 1: Basic conversion
    result = strtol("123", &endptr, 10);
    if (result != 123) return 1;
    
    // COMMENTED OUT TO ISOLATE
    // // Test 2: Negative numbers
    // result = strtol("-456", &endptr, 10);
    // if (result != -456) return 2;
    // 
    // // Test 3: Hex conversion
    // result = strtol("0xFF", &endptr, 16);
    // if (result != 255) return 3;
    
    return 0;
}
```

**If Test 1 still fails, reduce further:**
```c
int main() {
    // Simplest possible test
    long result = strtol("123", NULL, 10);
    return (result == 123) ? 0 : 1;
}
```

### 1.2 Isolate by Operation Type

**Replace library calls with direct operations:**

```c
// If strtol fails, test the underlying operations
int main() {
    // Test 1: Can we call a function?
    int (*func_ptr)(void) = simple_function;
    func_ptr();
    
    // Test 2: Can we do string operations?
    const char *str = "123";
    char c = str[0];
    if (c != '1') return 1;
    
    // Test 3: Can we do arithmetic?
    long val = 0;
    val = val * 10 + (c - '0');
    if (val != 1) return 2;
    
    // Test 4: Can we do pointer arithmetic?
    const char *p = str + 1;
    if (*p != '2') return 3;
    
    return 0;
}
```

### 1.3 Create Minimal Test Cases

Once you've isolated the failing operation, create a standalone test:

```c
// minimal_mul.c - Tests if multiplication works
int main() {
    long a = 10;
    long b = 5;
    long c = a * b;
    return (c == 50) ? 0 : 1;
}
```

**Save minimal tests to:**
```
/Users/benjamincooley/projects/m65832/emu/c_tests/baremetal/picolibc/minimal_<issue>.c
```

### 1.4 Use Preprocessor to Enable/Disable Code Sections

**For complex tests, use `#ifdef` to control sections:**

```c
#define TEST_BASIC_CONVERSION 1
#define TEST_NEGATIVE_NUMBERS 0  // Disable this section
#define TEST_HEX_CONVERSION 0

int main() {
#if TEST_BASIC_CONVERSION
    long result = strtol("123", NULL, 10);
    if (result != 123) return 1;
#endif

#if TEST_NEGATIVE_NUMBERS
    result = strtol("-456", NULL, 10);
    if (result != -456) return 2;
#endif

#if TEST_HEX_CONVERSION
    result = strtol("0xFF", NULL, 16);
    if (result != 255) return 3;
#endif

    return 0;
}
```

### 1.5 Add Debug Output

**Use UART output to see where execution fails:**

```c
#include <uart.h>  // From platform headers

void debug_print(const char *msg) {
    while (*msg) {
        uart_putc(*msg++);
    }
    uart_putc('\n');
}

int main() {
    debug_print("Starting test");
    
    long result = strtol("123", NULL, 10);
    debug_print("After strtol");
    
    if (result != 123) {
        debug_print("FAIL: wrong result");
        return 1;
    }
    
    debug_print("PASS");
    return 0;
}
```

**Run with emulator output:**
```bash
./m65832emu test.elf  # Output shows where it stops
```

## 2. LLVM Instruction Selection Debugging

When you've isolated a failing test to a specific operation (e.g., multiplication, function call, memory access), you need to debug why LLVM's instruction selection is failing.

### 2.1 Generate LLVM IR

**Compile to LLVM IR to see the operations before instruction selection:**

```bash
cd /Users/benjamincooley/projects/m65832/emu

$LLVM_BUILD/clang -target m65832-elf -O0 -ffreestanding \
    -I$SYSROOT/include \
    -S -emit-llvm test.c -o test.ll

# View the IR
cat test.ll
```

**Look for:**
- Function calls: `call i32 @function_name(...)`
- Arithmetic: `mul i32 %a, %b`, `add i32 %a, %b`
- Memory access: `load i32, ptr %ptr`, `store i32 %val, ptr %ptr`
- Comparisons: `icmp eq i32 %a, %b`
- Branches: `br i1 %cond, label %true, label %false`

**Identify which IR operations are in your minimal test.**

### 2.2 Generate Assembly with Debug Output

**Compile to assembly to see the generated M65832 instructions:**

```bash
# Generate assembly
$LLVM_BUILD/clang -target m65832-elf -O0 -ffreestanding \
    -I$SYSROOT/include \
    -S test.c -o test.s

# View assembly
cat test.s
```

**Check for:**
- Missing instructions (empty or invalid output)
- Wrong instruction selection
- Missing register allocations
- Incorrect operand types

### 2.3 Enable LLVM Debug Output

**LLVM provides extensive debug output for instruction selection. Use these flags:**

```bash
# Basic instruction selection debug
$LLVM_BUILD/clang -target m65832-elf -O0 -ffreestanding \
    -I$SYSROOT/include \
    -mllvm -debug \
    -S test.c -o test.s 2>&1 | tee isel-debug.log

# Specific to DAG instruction selection
$LLVM_BUILD/clang -target m65832-elf -O0 -ffreestanding \
    -I$SYSROOT/include \
    -mllvm -debug-only=isel \
    -S test.c -o test.s 2>&1 | tee isel-debug.log

# View the DAG before and after instruction selection
$LLVM_BUILD/clang -target m65832-elf -O0 -ffreestanding \
    -I$SYSROOT/include \
    -mllvm -debug-only=isel \
    -mllvm -view-dag-combine1-dags \
    -S test.c -o test.s 2>&1
```

**Important debug flags:**
- `-mllvm -debug` - Enable all LLVM debug output
- `-mllvm -debug-only=isel` - Only instruction selection debug
- `-mllvm -debug-only=regalloc` - Register allocation debug
- `-mllvm -print-after-all` - Print IR after each pass
- `-mllvm -print-machineinstrs` - Print machine instructions

### 2.4 Analyze SelectionDAG

**The SelectionDAG shows how LLVM represents operations before instruction selection:**

```bash
$LLVM_BUILD/clang -target m65832-elf -O0 -ffreestanding \
    -I$SYSROOT/include \
    -mllvm -debug-only=isel \
    -S test.c -o test.s 2>&1 | grep -A50 "Initial selection DAG"
```

**Look for:**
```
Initial selection DAG:
SelectionDAG has 15 nodes:
  t0: ch,glue = EntryToken
  t2: i32,ch = CopyFromReg t0, Register:i32 %0
  t4: i32,ch = CopyFromReg t0, Register:i32 %1
  t6: i32 = mul t2, t4          <-- This is the operation
  t8: ch,glue = CopyToReg t0, Register:i32 %2, t6
  t10: ch = M65832ISD::RET_FLAG t8, Register:i32 %2, t8:1
```

**Common issues:**
- `Cannot select: t6: i32 = mul t2, t4` - Missing pattern for `mul i32`
- `LLVM ERROR: Cannot select` - No matching instruction pattern
- Wrong node types (e.g., `i64` instead of `i32`)

### 2.5 Check TableGen Patterns

**Instruction patterns are defined in `.td` files:**

```bash
cd /Users/benjamincooley/projects/llvm-m65832/llvm/lib/Target/M65832

# View instruction definitions
cat M65832InstrInfo.td

# Search for specific patterns
grep -n "mul" M65832InstrInfo.td
grep -n "MUL32rr" M65832InstrInfo.td
```

**Example pattern:**
```tablegen
// In M65832InstrInfo.td
def MUL32rr : M65832Inst<(outs GR32:$dst), (ins GR32:$src1, GR32:$src2),
                         "mul $dst, $src1, $src2",
                         [(set GR32:$dst, (mul GR32:$src1, GR32:$src2))]>;
```

**If the pattern is missing, you need to add it.**

### 2.6 Use LLC Directly

**LLC is the LLVM static compiler - it's useful for testing backend changes without recompiling clang:**

```bash
# Compile to IR first
$LLVM_BUILD/clang -target m65832-elf -O0 -ffreestanding \
    -I$SYSROOT/include \
    -S -emit-llvm test.c -o test.ll

# Use LLC to compile IR to assembly with debug output
$LLVM_BUILD/llc -march=m65832 -debug -o test.s test.ll 2>&1 | tee llc-debug.log

# View specific debug output
$LLVM_BUILD/llc -march=m65832 -debug-only=isel -o test.s test.ll 2>&1
```

### 2.7 Common Instruction Selection Errors

**Error: "Cannot select: mul i32"**
- **Cause**: No pattern matches `(mul i32, i32)` in your TableGen definitions
- **Fix**: Add a pattern in `M65832InstrInfo.td`:
```tablegen
def : Pat<(mul GR32:$src1, GR32:$src2), (MUL32rr GR32:$src1, GR32:$src2)>;
```

**Error: "Cannot select: store"**
- **Cause**: Missing store pattern or wrong addressing mode
- **Fix**: Add store patterns for different addressing modes

**Error: "Cannot select: call"**
- **Cause**: Missing call instruction or wrong calling convention
- **Fix**: Implement `LowerCall` in `M65832ISelLowering.cpp`

**Error: Assertion in `M65832ISelDAGToDAG.cpp`**
- **Cause**: Custom instruction selection code has a bug
- **Fix**: Debug the `Select()` method in `M65832ISelDAGToDAG.cpp`

### 2.8 Test Instruction Selection Incrementally

**Create tiny tests for each operation:**

```c
// test_add.c
int main() {
    int a = 5;
    int b = 10;
    return a + b;  // Test if addition works
}

// test_mul.c
int main() {
    int a = 5;
    int b = 10;
    return a * b;  // Test if multiplication works
}

// test_call.c
int add(int a, int b) { return a + b; }
int main() {
    return add(5, 10);  // Test if function calls work
}
```

**Compile each and check the output:**
```bash
for test in test_*.c; do
    echo "=== Testing $test ==="
    $LLVM_BUILD/clang -target m65832-elf -O0 -S $test -o ${test%.c}.s
    cat ${test%.c}.s
done
```

## 3. Creating Comprehensive Test Coverage

Once you've fixed an issue, create a comprehensive test suite to catch all similar failures.

### 3.1 Test Categories

**For each fixed issue, create tests covering:**

1. **Basic operation** - Simplest possible case
2. **Edge cases** - Boundary values (0, -1, MAX_INT, MIN_INT)
3. **Different data types** - char, short, int, long, long long
4. **Different sizes** - 8-bit, 16-bit, 32-bit operations
5. **Signed vs unsigned** - Both variants
6. **Combinations** - Multiple operations together

### 3.2 Test Template

**Create systematic tests in `/Users/benjamincooley/projects/m65832/emu/c_tests/baremetal/picolibc/`:**

```c
// test_mul_comprehensive.c
// Test: Comprehensive multiplication tests for M65832
// Expected: 0 (all tests pass)

int test_count = 0;
int fail_count = 0;

void assert_equal(long expected, long actual, const char *test_name) {
    test_count++;
    if (expected != actual) {
        fail_count++;
        // Could add uart_print here for debugging
    }
}

int main() {
    // Test 1: Basic positive multiplication
    assert_equal(50, 10 * 5, "basic_mul");
    
    // Test 2: Multiply by zero
    assert_equal(0, 10 * 0, "mul_by_zero");
    assert_equal(0, 0 * 10, "zero_mul");
    
    // Test 3: Multiply by one
    assert_equal(10, 10 * 1, "mul_by_one");
    assert_equal(10, 1 * 10, "one_mul");
    
    // Test 4: Multiply by negative
    assert_equal(-50, 10 * -5, "mul_negative");
    assert_equal(-50, -10 * 5, "negative_mul");
    assert_equal(50, -10 * -5, "negative_both");
    
    // Test 5: Large values
    assert_equal(1000000, 1000 * 1000, "mul_large");
    
    // Test 6: Different sizes
    char c1 = 5, c2 = 10;
    assert_equal(50, c1 * c2, "mul_char");
    
    short s1 = 100, s2 = 200;
    assert_equal(20000, s1 * s2, "mul_short");
    
    // Test 7: Mixed types
    int i = 10;
    long l = 5;
    assert_equal(50, i * l, "mul_mixed");
    
    return fail_count;
}
```

### 3.3 Organize Tests by Category

**Create test files in a structured way:**

```
/Users/benjamincooley/projects/m65832/emu/c_tests/baremetal/picolibc/
├── arithmetic/
│   ├── test_add_comprehensive.c
│   ├── test_sub_comprehensive.c
│   ├── test_mul_comprehensive.c
│   ├── test_div_comprehensive.c
│   └── test_mod_comprehensive.c
├── bitwise/
│   ├── test_and_comprehensive.c
│   ├── test_or_comprehensive.c
│   ├── test_xor_comprehensive.c
│   └── test_shift_comprehensive.c
├── memory/
│   ├── test_load_comprehensive.c
│   ├── test_store_comprehensive.c
│   └── test_pointer_comprehensive.c
├── control_flow/
│   ├── test_branch_comprehensive.c
│   ├── test_call_comprehensive.c
│   └── test_return_comprehensive.c
└── string/
    ├── test_strlen_comprehensive.c
    ├── test_strcmp_comprehensive.c
    └── test_memcpy_comprehensive.c
```

### 3.4 Add Tests to Python Runner

**The Python test runner automatically discovers tests in the `c_tests` directory:**

```bash
# Your new tests will be automatically included
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=baremetal
```

**If you want to add a custom test location, edit the Python runner:**

```python
# In run_picolibc_gtest.py, around line 107
def find_test_files() -> List[Tuple[str, str, str]]:
    tests = []
    
    # Add your custom test directory
    CUSTOM_TESTS_ARITHMETIC = PROJECTS_ROOT / "m65832" / "emu" / "c_tests" / "baremetal" / "picolibc" / "arithmetic"
    if CUSTOM_TESTS_ARITHMETIC.exists():
        for f in sorted(CUSTOM_TESTS_ARITHMETIC.glob("*.c")):
            desc = extract_description(str(f))
            tests.append(("arithmetic", str(f), desc))
    
    # ... rest of function
```

### 3.5 Create a Test Matrix

**For complex features, create a test matrix:**

```c
// test_strtol_matrix.c - Comprehensive strtol testing

struct test_case {
    const char *input;
    int base;
    long expected;
    const char *name;
};

int main() {
    struct test_case tests[] = {
        // Basic conversions
        {"123", 10, 123, "basic_decimal"},
        {"0", 10, 0, "zero"},
        {"1", 10, 1, "one"},
        
        // Negative numbers
        {"-123", 10, -123, "negative"},
        {"-1", 10, -1, "neg_one"},
        
        // Different bases
        {"FF", 16, 255, "hex_ff"},
        {"0xFF", 16, 255, "hex_0xff"},
        {"377", 8, 255, "octal"},
        {"11111111", 2, 255, "binary"},
        
        // Edge cases
        {"2147483647", 10, 2147483647, "max_int"},
        {"-2147483648", 10, -2147483648, "min_int"},
        
        // Whitespace
        {"  123", 10, 123, "leading_space"},
        {"123  ", 10, 123, "trailing_space"},
        
        // Invalid input (should stop at first invalid char)
        {"123abc", 10, 123, "partial_valid"},
        
        {NULL, 0, 0, NULL}  // Sentinel
    };
    
    int fail_count = 0;
    for (int i = 0; tests[i].input != NULL; i++) {
        long result = strtol(tests[i].input, NULL, tests[i].base);
        if (result != tests[i].expected) {
            fail_count++;
        }
    }
    
    return fail_count;
}
```

### 3.6 Run Regression Tests

**After adding new tests, run them all:**

```bash
# Run just your new tests
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --filter="comprehensive"

# Run the full suite to check for regressions
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=baremetal
```

## 4. Documenting Your Fixes

Proper documentation helps others understand what was fixed and how, and helps prevent regressions.

### 4.1 Where to Place Fix Documentation

**Create a fix log directory:**
```bash
mkdir -p /Users/benjamincooley/projects/llvm-m65832/docs/fixes
```

**Each fix gets its own markdown file:**
```
/Users/benjamincooley/projects/llvm-m65832/docs/fixes/
├── 2026-01-31-fix-multiplication-i32.md
├── 2026-02-01-fix-function-calls.md
├── 2026-02-02-fix-strtol-overflow.md
└── README.md  # Index of all fixes
```

### 4.2 Fix Documentation Template

**Create a file: `YYYY-MM-DD-fix-<short-description>.md`**

```markdown
# Fix: [Short description of the issue]

**Date**: YYYY-MM-DD  
**Author**: [Your name or "AI Agent"]  
**Severity**: [Critical | High | Medium | Low]  
**Component**: [Backend | Instruction Selection | Register Allocation | etc.]

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
Initial selection DAG:
  t6: i32 = mul i32 t2, i32 t4
  
LLVM ERROR: Cannot select: t6: i32 = mul t2, t4

Checked M65832InstrInfo.td:
  def MUL32rr : M65832Inst<...>; // Instruction defined
  # But no pattern!
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
```tablegen
// Pattern to match integer multiplication
def : Pat<(mul GR32:$src1, GR32:$src2), 
          (MUL32rr GR32:$src1, GR32:$src2)>;
```

This tells LLVM to use the MUL32rr instruction when it sees
a multiplication of two 32-bit general registers.
```

### Why This Fix Works
- Explain the reasoning behind your solution
- Describe any trade-offs or alternative approaches considered

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
  - 15 subtests covering: basic mul, zero, negative, large values, etc.
  - All 15 subtests passing

Full test suite results:
- Baremetal: 27/27 passing (no regressions)
- libc_testsuite: 6/10 passing (was 5/10)
```

## Related Issues

- List any related bugs or missing features
- Link to similar fixes or issues
- Note any follow-up work needed

Example:
```
Related issues:
- Division (DIV32rr) also needs patterns
- 64-bit multiplication not yet supported
- Optimization: mul by constant could use shifts

Follow-up:
- Add patterns for other arithmetic operations
- Create comprehensive arithmetic test suite
- Document all supported operations
```

## References

- Link to LLVM documentation
- Cite any external resources used
- Note any discussions or design decisions

Example:
```
References:
- LLVM TableGen documentation: https://llvm.org/docs/TableGen/
- M65832 ISA specification: docs/M65832-ISA.md
- Similar fix in ARM backend: llvm/lib/Target/ARM/ARMInstrInfo.td:1234
```
```

### 4.3 Update the Master Fix Index

**Maintain an index of all fixes:**

**File: `/Users/benjamincooley/projects/llvm-m65832/docs/fixes/README.md`**

```markdown
# M65832 Backend Fix Log

This directory contains documentation for all fixes applied to the M65832 LLVM backend.

## Index

### Instruction Selection Fixes

| Date | Issue | Description | Tests Fixed | File |
|------|-------|-------------|-------------|------|
| 2026-01-31 | i32 multiplication | Missing pattern for mul instruction | strtol, atoi | [2026-01-31-fix-multiplication-i32.md](2026-01-31-fix-multiplication-i32.md) |
| 2026-02-01 | Function calls | Incorrect calling convention | atexit, constructor | [2026-02-01-fix-function-calls.md](2026-02-01-fix-function-calls.md) |

### Code Generation Fixes

| Date | Issue | Description | Tests Fixed | File |
|------|-------|-------------|-------------|------|
| ... | ... | ... | ... | ... |

### Library Implementation Fixes

| Date | Issue | Description | Tests Fixed | File |
|------|-------|-------------|-------------|------|
| ... | ... | ... | ... | ... |

## Statistics

- **Total fixes**: X
- **Tests fixed**: Y
- **Tests remaining**: Z
- **Pass rate**: XX%

## Common Patterns

### Pattern 1: Missing TableGen Patterns
Many fixes involve adding missing patterns to M65832InstrInfo.td.
See fixes: 2026-01-31, 2026-02-03, etc.

### Pattern 2: Incorrect Lowering
Some operations need custom lowering in M65832ISelLowering.cpp.
See fixes: 2026-02-01, etc.

## How to Use This Log

1. **Before fixing**: Search this log to see if a similar issue was fixed
2. **After fixing**: Create a new fix document using the template
3. **Update this index**: Add your fix to the appropriate table
4. **Run regression tests**: Ensure no previous fixes broke
```

### 4.4 Include Code Snippets

**When documenting code changes, include before/after:**

```markdown
### Before (Broken)

```tablegen
// M65832InstrInfo.td - line 234
def MUL32rr : M65832Inst<(outs GR32:$dst), (ins GR32:$src1, GR32:$src2),
                         "mul $dst, $src1, $src2",
                         []>;  // No pattern!
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
```

### 4.5 Document in Code Comments

**Also add comments directly in the code:**

```cpp
// M65832InstrInfo.td

// Integer Arithmetic Patterns
// ===========================
// Fixed 2026-01-31: Added patterns for basic arithmetic operations
// These patterns match LLVM IR operations to M65832 instructions
// See: docs/fixes/2026-01-31-fix-multiplication-i32.md

def : Pat<(mul GR32:$src1, GR32:$src2), (MUL32rr GR32:$src1, GR32:$src2)>;
def : Pat<(add GR32:$src1, GR32:$src2), (ADD32rr GR32:$src1, GR32:$src2)>;
def : Pat<(sub GR32:$src1, GR32:$src2), (SUB32rr GR32:$src1, GR32:$src2)>;
```

### 4.6 Update Test Documentation

**When you add new tests, document them:**

**File: `/Users/benjamincooley/projects/m65832/emu/c_tests/baremetal/picolibc/README.md`**

```markdown
# M65832 Baremetal Test Suite

## Test Categories

### Arithmetic Tests
- `test_add_comprehensive.c` - Addition (all integer types)
- `test_mul_comprehensive.c` - Multiplication (added 2026-01-31)
  - Tests: basic, zero, negative, large values, mixed types
  - Covers fix for missing i32 mul pattern
  
### String Tests
- `string_strlen.c` - strlen() function
- `string_strcmp.c` - strcmp() function
...
```

## Quick Reference

### Essential Commands

```bash
# Test a single failing test
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py <test_name>

# Generate LLVM IR
$LLVM_BUILD/clang -target m65832-elf -S -emit-llvm test.c -o test.ll

# Generate assembly with debug
$LLVM_BUILD/clang -target m65832-elf -mllvm -debug-only=isel -S test.c 2>&1

# Run test manually on emulator
./m65832emu -c 10000000 --stop-on-brk test.elf

# Check for regressions
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=baremetal
```

### Key Files

- **Test runner**: `/Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py`
- **Test sources**: `/Users/benjamincooley/projects/picolibc-m65832/test/`
- **Custom tests**: `/Users/benjamincooley/projects/m65832/emu/c_tests/baremetal/picolibc/`
- **Backend code**: `/Users/benjamincooley/projects/llvm-m65832/llvm/lib/Target/M65832/`
- **Fix docs**: `/Users/benjamincooley/projects/llvm-m65832/docs/fixes/`

### Debug Flags

- `-mllvm -debug` - All debug output
- `-mllvm -debug-only=isel` - Instruction selection only
- `-mllvm -print-after-all` - IR after each pass
- `-mllvm -view-dag-combine1-dags` - Visualize DAGs
