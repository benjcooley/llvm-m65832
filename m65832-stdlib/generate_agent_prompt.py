#!/usr/bin/env python3
"""
Generate a ready-to-use prompt for an LLM agent to fix a specific M65832 picolibc test.

Usage: ./generate_agent_prompt.py <test_name>
Example: ./generate_agent_prompt.py strtol
"""

import sys

PROMPT_TEMPLATE = """# M65832 Bug Fix Mission: {test_name}

You are an expert LLVM backend developer working on the M65832 architecture. Your task is to fix the failing picolibc test: **{test_name}**

## Essential Documentation (READ FIRST!)
1. `/Users/benjamincooley/projects/llvm-m65832/m65832-stdlib/QUICK-REFERENCE.md` - Commands and locations
2. `/Users/benjamincooley/projects/llvm-m65832/m65832-stdlib/PICOLIBC_TEST_STATUS.md` - Complete workflow (Sections 0-4)

## Your Workflow

### Step 1: Understand the Current State
```bash
cd /Users/benjamincooley/projects/m65832/emu
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py {test_name}
```

### Step 2: Locate and Read the Test Source
```bash
find /Users/benjamincooley/projects/picolibc-m65832/test -name "*{test_name}*"
```

### Step 3: Isolate the Failure
Follow Section 1 of PICOLIBC_TEST_STATUS.md:
- Create a minimal test case that reproduces the failure
- Use binary search with commenting to isolate the exact operation that fails
- Save minimal test to: `/Users/benjamincooley/projects/m65832/emu/c_tests/baremetal/picolibc/minimal_{test_name}.c`

### Step 4: Debug with LLVM
Follow Section 2 of PICOLIBC_TEST_STATUS.md:
```bash
# Generate LLVM IR
export LLVM_BUILD=/Users/benjamincooley/projects/llvm-m65832/build-fast/bin
export SYSROOT=/Users/benjamincooley/projects/m65832-sysroot

$LLVM_BUILD/clang -target m65832-elf -S -emit-llvm -I$SYSROOT/include test.c -o test.ll

# Debug instruction selection
$LLVM_BUILD/clang -target m65832-elf -mllvm -debug-only=isel -I$SYSROOT/include -S test.c 2>&1 | tee isel_debug.log
```

Identify which IR operation fails (mul, call, load, store, etc.)

### Step 5: Implement the Fix
Common fixes:
- **"Cannot select: <op>"** → Add pattern to `/Users/benjamincooley/projects/llvm-m65832/llvm/lib/Target/M65832/M65832InstrInfo.td`
- **"undefined symbol"** → Implement in libcompiler_rt or check linking
- **Timeout/infinite loop** → Debug generated assembly, may be miscompilation
- **Wrong result** → Check instruction selection, may need custom lowering

### Step 6: Create Comprehensive Tests
Follow Section 3 of PICOLIBC_TEST_STATUS.md:
- Create test file: `/Users/benjamincooley/projects/m65832/emu/c_tests/baremetal/picolibc/test_{test_name}_comprehensive.c`
- Cover: basic case, edge cases, different types, signed/unsigned

### Step 7: Validate (CRITICAL!)
```bash
# Test your fix
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py {test_name}

# Check for regressions - THIS IS MANDATORY
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=baremetal
```
**All 27 baremetal tests must still pass!**

### Step 8: Document Your Fix
Follow Section 4 of PICOLIBC_TEST_STATUS.md:
1. Create: `/Users/benjamincooley/projects/llvm-m65832/docs/fixes/$(date +%Y-%m-%d)-fix-{test_name}.md`
2. Use template: `/Users/benjamincooley/projects/llvm-m65832/docs/fixes/FIX-TEMPLATE.md`
3. Update: `/Users/benjamincooley/projects/llvm-m65832/docs/fixes/README.md` index

## Success Criteria
- [ ] Test passes: `python3 run_picolibc_gtest.py {test_name}` shows PASS
- [ ] No regressions: `--suite=baremetal` still shows 27/27 PASSING
- [ ] Comprehensive tests created
- [ ] Fix documented in docs/fixes/
- [ ] Ready to commit

## Key Locations
- **Backend code**: `/Users/benjamincooley/projects/llvm-m65832/llvm/lib/Target/M65832/`
- **Test runner**: `/Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py`
- **Compiler**: `/Users/benjamincooley/projects/llvm-m65832/build-fast/bin/clang`
- **Sysroot**: `/Users/benjamincooley/projects/m65832-sysroot`

## Remember
- Start with the documentation (QUICK-REFERENCE.md and PICOLIBC_TEST_STATUS.md)
- Create minimal test cases
- Use LLVM debug output extensively
- Always check for regressions
- Document everything

**Now begin working on fixing: {test_name}**
"""

def main():
    if len(sys.argv) != 2 or sys.argv[1] in ['-h', '--help']:
        print(__doc__)
        print("\nAvailable tests (from FAILING-TESTS-LIST.md):")
        print("  High Priority: strtol, strtod, sscanf, wcstol, atexit")
        print("  Performance:   snprintf, ffs, malloc, malloc_stress, math-funcs")
        print("  Advanced:      constructor")
        sys.exit(0)
    
    test_name = sys.argv[1]
    
    # Remove common prefixes if present
    test_name = test_name.replace('libc_testsuite.', '')
    test_name = test_name.replace('picolibc.', '')
    test_name = test_name.replace('baremetal.', '')
    
    prompt = PROMPT_TEMPLATE.format(test_name=test_name)
    print(prompt)

if __name__ == '__main__':
    main()
