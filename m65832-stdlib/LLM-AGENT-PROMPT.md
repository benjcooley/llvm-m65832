# LLM Agent Bug Fix Prompt

You are an expert LLVM backend developer working on the M65832 architecture. Your task is to fix a failing picolibc test by debugging and fixing the M65832 LLVM backend.

## Your Mission
Fix the following picolibc test: **[TEST_NAME_WILL_BE_INSERTED_HERE]**

## Essential Documentation
Before starting, READ these documents in order:
1. `/Users/benjamincooley/projects/llvm-m65832/m65832-stdlib/QUICK-REFERENCE.md` - Commands and locations
2. `/Users/benjamincooley/projects/llvm-m65832/m65832-stdlib/PICOLIBC_TEST_STATUS.md` - Complete workflow (Section 0-4)

## Your Workflow

### Step 1: Understand the Current State
```bash
cd /Users/benjamincooley/projects/m65832/emu
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py [TEST_NAME]
```

### Step 2: Locate and Read the Test Source
```bash
find /Users/benjamincooley/projects/picolibc-m65832/test -name "*[TEST_NAME]*"
```

### Step 3: Isolate the Failure
Follow Section 1 of PICOLIBC_TEST_STATUS.md:
- Create a minimal test case that reproduces the failure
- Use binary search with commenting to isolate the exact operation that fails
- Save minimal test to: `/Users/benjamincooley/projects/m65832/emu/c_tests/baremetal/picolibc/minimal_[issue].c`

### Step 4: Debug with LLVM
Follow Section 2 of PICOLIBC_TEST_STATUS.md:
- Generate LLVM IR to see operations before instruction selection
- Use `-mllvm -debug-only=isel` to see instruction selection errors
- Identify which IR operation fails (mul, call, load, store, etc.)
- Check if TableGen patterns are missing in `M65832InstrInfo.td`

### Step 5: Implement the Fix
Common fixes:
- **"Cannot select: <op>"** → Add pattern to `/Users/benjamincooley/projects/llvm-m65832/llvm/lib/Target/M65832/M65832InstrInfo.td`
- **"undefined symbol"** → Implement in libcompiler_rt or check linking
- **Timeout/infinite loop** → Debug generated assembly, may be miscompilation
- **Wrong result** → Check instruction selection, may need custom lowering

### Step 6: Create Comprehensive Tests
Follow Section 3 of PICOLIBC_TEST_STATUS.md:
- Create test file: `/Users/benjamincooley/projects/m65832/emu/c_tests/baremetal/picolibc/test_[feature]_comprehensive.c`
- Cover: basic case, edge cases, different types, signed/unsigned

### Step 7: Validate (Critical!)
```bash
# Test your fix
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py [TEST_NAME]

# Check for regressions - THIS IS MANDATORY
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=baremetal
```
**All 27 baremetal tests must still pass!**

### Step 8: Document Your Fix
Follow Section 4 of PICOLIBC_TEST_STATUS.md:
1. Create: `/Users/benjamincooley/projects/llvm-m65832/docs/fixes/YYYY-MM-DD-fix-[description].md`
2. Use template: `/Users/benjamincooley/projects/llvm-m65832/docs/fixes/FIX-TEMPLATE.md`
3. Update: `/Users/benjamincooley/projects/llvm-m65832/docs/fixes/README.md` index

## Key Locations
- **Backend code**: `/Users/benjamincooley/projects/llvm-m65832/llvm/lib/Target/M65832/`
- **Test runner**: `/Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py`
- **Compiler**: `/Users/benjamincooley/projects/llvm-m65832/build-fast/bin/clang`
- **Sysroot**: `/Users/benjamincooley/projects/m65832-sysroot`

## Success Criteria
- [ ] Test passes: `python3 run_picolibc_gtest.py [TEST_NAME]` shows PASS
- [ ] No regressions: `--suite=baremetal` still shows 27/27 PASSING
- [ ] Comprehensive tests created
- [ ] Fix documented in docs/fixes/
- [ ] Ready to commit

## Debug Commands Quick Reference
```bash
# Generate IR
$LLVM_BUILD/clang -target m65832-elf -S -emit-llvm test.c -o test.ll

# Debug instruction selection
$LLVM_BUILD/clang -target m65832-elf -mllvm -debug-only=isel -S test.c 2>&1

# Generate assembly
$LLVM_BUILD/clang -target m65832-elf -S test.c -o test.s
```

## Remember
- Start with the documentation (QUICK-REFERENCE.md and PICOLIBC_TEST_STATUS.md)
- Create minimal test cases
- Use LLVM debug output extensively
- Always check for regressions
- Document everything

Now begin working on: **[TEST_NAME_WILL_BE_INSERTED_HERE]**
