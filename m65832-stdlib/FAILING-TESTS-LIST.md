# M65832 Picolibc Failing Tests - Fix List

## Instructions for Use
1. Copy the LLM-AGENT-PROMPT.md content
2. Select one test from the list below
3. Replace `[TEST_NAME_WILL_BE_INSERTED_HERE]` with the test name (twice in the prompt)
4. Give the complete prompt to your LLM agent

---

## Failing Tests by Priority

### HIGH PRIORITY - Core Functionality (5 tests)

1. **libc_testsuite.strtol**
   - Category: String conversion
   - Symptom: Returns wrong value (4294967264)
   - Expected: Parse string to long integer
   - Likely Issue: Overflow handling or arithmetic bug

2. **libc_testsuite.strtod**
   - Category: String conversion
   - Symptom: Returns wrong value (1047744)
   - Expected: Parse string to double
   - Likely Issue: Floating point conversion

3. **libc_testsuite.sscanf**
   - Category: String parsing
   - Symptom: Returns wrong value (1046092)
   - Expected: Parse formatted input
   - Likely Issue: Complex parsing logic or varargs

4. **libc_testsuite.wcstol**
   - Category: Wide character conversion
   - Symptom: Unknown (timed out in test run)
   - Expected: Parse wide string to long
   - Likely Issue: Wide character support or similar to strtol

5. **picolibc.atexit**
   - Category: Program termination
   - Symptom: Returns 1 (should return 0)
   - Expected: Register exit handlers
   - Likely Issue: Function pointer handling or calling convention

### MEDIUM PRIORITY - Performance Issues (5 tests)

6. **libc_testsuite.snprintf**
   - Category: String formatting
   - Symptom: Timeout (30+ seconds)
   - Expected: Format string output
   - Likely Issue: Infinite loop or very slow execution

7. **picolibc.ffs**
   - Category: Bit operations
   - Symptom: Timeout (30+ seconds)
   - Expected: Find first set bit
   - Likely Issue: Missing instruction or infinite loop

8. **picolibc.malloc**
   - Category: Memory allocation
   - Symptom: Timeout (30+ seconds)
   - Expected: Allocate heap memory
   - Likely Issue: Heap management bug or infinite loop

9. **picolibc.malloc_stress**
   - Category: Memory allocation
   - Symptom: Timeout (30+ seconds)
   - Expected: Stress test malloc/free
   - Likely Issue: Same as malloc test

10. **picolibc.math-funcs**
    - Category: Math operations
    - Symptom: Timeout (30+ seconds)
    - Expected: Various math functions
    - Likely Issue: Floating point operations or library issue

### LOWER PRIORITY - Advanced Features (3 tests)

11. **picolibc.constructor**
    - Category: C++ support
    - Symptom: Returns 1 (should return 0)
    - Expected: Run global constructors
    - Likely Issue: Constructor support or section handling

12. **test_stdio.printf_float** (if it exists in suite)
    - Category: Formatted I/O
    - Symptom: Likely formatting or float conversion
    - Expected: Print floating point numbers
    - Likely Issue: Float-to-string conversion

13. **test_math.trigonometric** (if it exists in suite)
    - Category: Math library
    - Symptom: Likely incorrect results
    - Expected: sin, cos, tan functions
    - Likely Issue: Math library implementation

---

## Test Suite Status Summary

- **Total Tests**: 179 tests across 9 suites
- **Passing**: 27/27 baremetal tests (100% core functionality)
- **High Priority Failures**: 5 tests (core C library functionality)
- **Performance Issues**: 5 tests (timeouts, likely infinite loops)
- **Lower Priority**: 3+ tests (advanced features)

## Recommended Order

**Start with these in order:**
1. strtol (most basic string conversion)
2. atexit (simpler than constructor)
3. ffs (single operation, easier to debug)
4. constructor (C++ support)
5. malloc (complex but important)

**Then tackle timeouts:**
- snprintf, malloc_stress, math-funcs (require performance debugging)

## Notes

- **Before starting**: All agents must read QUICK-REFERENCE.md and PICOLIBC_TEST_STATUS.md
- **Success metric**: Test passes AND no regressions in baremetal suite
- **Documentation required**: Every fix must be documented in docs/fixes/
- **Comprehensive tests**: Create full test coverage for each fixed feature

## Quick Test Command

```bash
cd /Users/benjamincooley/projects/m65832/emu
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py [TEST_NAME]
```

## Regression Check (MANDATORY after every fix)

```bash
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=baremetal
# Must show: [  PASSED  ] 27 tests.
```
