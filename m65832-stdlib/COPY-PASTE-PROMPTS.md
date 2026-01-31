# Copy-Paste Ready: LLM Agent Prompts for M65832 Bug Fixes

## How to Use This Document

1. **Scroll down** to the "Individual Test Prompts" section
2. **Copy** the entire prompt for the test you want to fix (from the header to the end)
3. **Paste** into your LLM agent conversation
4. The agent will have everything needed to fix that specific test

## Quick Command Reference

Generate a prompt automatically:
```bash
cd /Users/benjamincooley/projects/llvm-m65832/m65832-stdlib
./generate_agent_prompt.py strtol
```

Or pick from the pre-generated prompts below.

---

# Individual Test Prompts

## Test 1: strtol (String to Long Conversion)

Copy from here ↓↓↓

---

# M65832 Bug Fix Mission: strtol

You are an expert LLVM backend developer working on the M65832 architecture. Your task is to fix the failing picolibc test: **strtol**

**Test Info**: String to long integer conversion (High Priority - Core Functionality)

## Essential Documentation (READ FIRST!)
1. `/Users/benjamincooley/projects/llvm-m65832/m65832-stdlib/QUICK-REFERENCE.md` - Commands and locations
2. `/Users/benjamincooley/projects/llvm-m65832/m65832-stdlib/PICOLIBC_TEST_STATUS.md` - Complete workflow (Sections 0-4)

## Your Workflow

### Step 1: Understand the Current State
```bash
cd /Users/benjamincooley/projects/m65832/emu
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py strtol
```

### Step 2: Locate and Read the Test Source
```bash
find /Users/benjamincooley/projects/picolibc-m65832/test -name "*strtol*"
# Read the test file to understand what it does
```

### Step 3: Isolate the Failure (Section 1 of PICOLIBC_TEST_STATUS.md)
- Create a minimal test case that reproduces the failure
- Use binary search with commenting to isolate the exact operation
- Save to: `/Users/benjamincooley/projects/m65832/emu/c_tests/baremetal/picolibc/minimal_strtol.c`

### Step 4: Debug with LLVM (Section 2 of PICOLIBC_TEST_STATUS.md)
```bash
export LLVM_BUILD=/Users/benjamincooley/projects/llvm-m65832/build-fast/bin
export SYSROOT=/Users/benjamincooley/projects/m65832-sysroot

# Generate LLVM IR
$LLVM_BUILD/clang -target m65832-elf -S -emit-llvm -I$SYSROOT/include test.c -o test.ll

# Debug instruction selection
$LLVM_BUILD/clang -target m65832-elf -mllvm -debug-only=isel -I$SYSROOT/include -S test.c 2>&1 | tee isel_debug.log
```

### Step 5: Implement the Fix
Check for:
- Missing TableGen patterns in `M65832InstrInfo.td`
- Arithmetic operations (mul, div, mod for string parsing)
- Comparison operations
- Branch operations

### Step 6: Create Comprehensive Tests (Section 3)
Create: `/Users/benjamincooley/projects/m65832/emu/c_tests/baremetal/picolibc/test_strtol_comprehensive.c`
Cover: basic, negative, hex, octal, binary, edge cases (0, MAX_INT, MIN_INT)

### Step 7: Validate (CRITICAL!)
```bash
# Test your fix
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py strtol

# MANDATORY regression check
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=baremetal
```
**All 27 baremetal tests MUST still pass!**

### Step 8: Document Your Fix (Section 4)
1. Create: `/Users/benjamincooley/projects/llvm-m65832/docs/fixes/$(date +%Y-%m-%d)-fix-strtol.md`
2. Use template: `/Users/benjamincooley/projects/llvm-m65832/docs/fixes/FIX-TEMPLATE.md`
3. Update: `/Users/benjamincooley/projects/llvm-m65832/docs/fixes/README.md`

## Success Criteria
- [ ] `python3 run_picolibc_gtest.py strtol` → PASS
- [ ] `--suite=baremetal` → 27/27 PASSING (no regressions)
- [ ] Comprehensive test created
- [ ] Fix documented
- [ ] Ready to commit

**Now begin working on fixing: strtol**

---

Copy to here ↑↑↑

---

## Test 2: atexit (Exit Handler Registration)

Copy from here ↓↓↓

---

# M65832 Bug Fix Mission: atexit

You are an expert LLVM backend developer working on the M65832 architecture. Your task is to fix the failing picolibc test: **atexit**

**Test Info**: Exit handler registration (High Priority - Returns 1 instead of 0)

## Essential Documentation (READ FIRST!)
1. `/Users/benjamincooley/projects/llvm-m65832/m65832-stdlib/QUICK-REFERENCE.md`
2. `/Users/benjamincooley/projects/llvm-m65832/m65832-stdlib/PICOLIBC_TEST_STATUS.md` (Sections 0-4)

## Your Workflow

### Step 1: Understand the Current State
```bash
cd /Users/benjamincooley/projects/m65832/emu
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py atexit
```

### Step 2-8: Follow the complete workflow from PICOLIBC_TEST_STATUS.md Section 0

Key areas to check for atexit:
- Function pointer handling
- Calling conventions
- Program termination flow
- Memory allocation for exit handler list

### Validation (CRITICAL!)
```bash
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py atexit
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=baremetal
```

**Now begin working on fixing: atexit**

---

Copy to here ↑↑↑

---

## Test 3-11: Use the Generator Script

For remaining tests, use:
```bash
cd /Users/benjamincooley/projects/llvm-m65832/m65832-stdlib
./generate_agent_prompt.py <test_name>
```

Available tests:
- strtod
- sscanf  
- wcstol
- snprintf
- ffs
- malloc
- malloc_stress
- math-funcs
- constructor

---

## All Tests Summary (for reference)

**HIGH PRIORITY:**
1. strtol - String to long conversion
2. strtod - String to double conversion
3. sscanf - Formatted string parsing
4. wcstol - Wide char to long conversion
5. atexit - Exit handler registration

**PERFORMANCE (Timeouts):**
6. snprintf - String formatting
7. ffs - Find first set bit
8. malloc - Memory allocation
9. malloc_stress - Malloc stress test
10. math-funcs - Math functions

**ADVANCED:**
11. constructor - Global constructors

---

## Quick Tips for All Tests

**Before starting:**
- Read QUICK-REFERENCE.md
- Read PICOLIBC_TEST_STATUS.md Sections 0-4

**While working:**
- Create minimal test cases
- Use `-mllvm -debug-only=isel` extensively
- Check TableGen patterns first

**After fixing:**
- Always run regression tests: `--suite=baremetal`
- Document in docs/fixes/
- Create comprehensive tests

**Success = Test passes + No regressions + Documentation**
