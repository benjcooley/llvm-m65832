# M65832 Backend Fix Log

This directory contains documentation for all fixes applied to the M65832 LLVM backend.

## Index

### Instruction Selection Fixes

| Date | Issue | Description | Tests Fixed | File |
|------|-------|-------------|-------------|------|
| 2026-01-31 | disjoint OR in vsprintf | Treat disjoint OR as ADD for pointer arithmetic | stdio O2 hang | [2026-01-31-disjoint-or-vsprintf.md](2026-01-31-disjoint-or-vsprintf.md) |
| 2026-01-31 | memmove OR address | Accept OR(FrameIndex, const) in selectAddr | memmove @<noreg> | [2026-01-31-memmove-or-addr.md](2026-01-31-memmove-or-addr.md) |
| 2026-01-31 | frameindex base addr | Use TargetFrameIndex for base+offset | stdio O2 hang | [2026-01-31-frameindex-base-addr.md](2026-01-31-frameindex-base-addr.md) |
| 2026-01-31 | STORE32 $noreg | Immediate store drops value in ISel | test_64bit_basic/divide (ongoing) | [2026-01-31-store32-noreg-isel.md](2026-01-31-store32-noreg-isel.md) |
| 2026-01-30 | shift count 31 | Avoid reserved 0x1F immediate shift encoding | test_64bit (one << 63) | [2026-01-30-shift-imm-31.md](2026-01-30-shift-imm-31.md) |

### Code Generation Fixes

| Date | Issue | Description | Tests Fixed | File |
|------|-------|-------------|-------------|------|
| _Example_ | Register spilling | Incorrect stack frame layout | complex tests | [YYYY-MM-DD-fix-description.md](YYYY-MM-DD-fix-description.md) |

### Library Implementation Fixes

| Date | Issue | Description | Tests Fixed | File |
|------|-------|-------------|-------------|------|
| _Example_ | strtol overflow | Incorrect handling of MAX_INT | strtol edge cases | [YYYY-MM-DD-fix-description.md](YYYY-MM-DD-fix-description.md) |
| 2026-02-01 | 64-bit udiv/umod | Fix __udivdi3/__umoddi3 runtime path | test_64bit_divide | [2026-02-01-udivmod64-runtime.md](2026-02-01-udivmod64-runtime.md) |

## Statistics

- **Total fixes**: 0 (just started)
- **Picolibc tests passing**: 27/179 baremetal tests (100% of core tests)
- **Tests remaining**: ~15-20 issues to fix
- **Current pass rate**: 15% overall, 100% of basic functionality

## Common Issue Patterns

As you fix issues, document common patterns here to help identify similar problems faster.

### Pattern 1: Missing TableGen Patterns
**Symptom**: `LLVM ERROR: Cannot select: <operation>`  
**Cause**: Instruction defined but no pattern to match IR operation  
**Fix**: Add pattern in `M65832InstrInfo.td`  
**See fixes**: (add links as you create them)

### Pattern 2: Incorrect Calling Convention
**Symptom**: Function calls fail, wrong values in registers  
**Cause**: Incorrect implementation in `M65832CallingConv.td` or `M65832ISelLowering.cpp`  
**Fix**: Implement proper `LowerCall` and `LowerFormalArguments`  
**See fixes**: (add links as you create them)

### Pattern 3: Missing Library Functions
**Symptom**: Link error: undefined symbol  
**Cause**: Function not implemented in compiler runtime or picolibc  
**Fix**: Add implementation to libcompiler_rt or picolibc port  
**See fixes**: (add links as you create them)

### Pattern 4: Infinite Loops (Timeouts)
**Symptom**: Test times out after 30 seconds  
**Cause**: Miscompilation causing infinite loop, or very slow unoptimized code  
**Fix**: Debug with emulator, check generated assembly, may need optimization  
**See fixes**: (add links as you create them)

## How to Use This Log

### Before Fixing an Issue

1. **Search this log**: Check if a similar issue was already fixed
2. **Check common patterns**: Identify which category your issue falls into
3. **Read related fixes**: Learn from similar solutions

### While Fixing an Issue

1. **Follow the workflow**: See main document sections 0-3
2. **Document as you go**: Take notes on your investigation
3. **Save debug output**: Keep LLVM debug logs and test results

### After Fixing an Issue

1. **Create a fix document**: Use the template in the main document (Section 4.2)
2. **Update this index**: Add your fix to the appropriate table above
3. **Update common patterns**: If you found a new pattern, document it
4. **Run regression tests**: Ensure no previous fixes broke
5. **Commit with reference**: Include fix document path in commit message

## Template Files

- **Fix documentation template**: See section 4.2 in `../PICOLIBC_TEST_STATUS.md`
- **Test template**: See section 3.2 in `../PICOLIBC_TEST_STATUS.md`

## Related Documentation

- **Test Status**: `../PICOLIBC_TEST_STATUS.md` - Current test results and workflow
- **Backend Architecture**: `../M65832-Backend-Architecture.md` (if exists)
- **ISA Specification**: `../M65832-ISA.md` (if exists)
- **Calling Convention**: `../M65832-Calling-Convention.md` (if exists)

## Questions or Need Help?

When stuck on an issue:

1. **Check LLVM documentation**: https://llvm.org/docs/
2. **Look at similar targets**: Check ARM, RISC-V, or Mips backends for examples
3. **Use LLVM debug output**: See section 2 of main document for debug techniques
4. **Create minimal test case**: Isolate the problem (section 1 of main document)

## Progress Tracking

### High Priority Issues (Core Functionality)

- [ ] String conversion functions (strtol, strtod, sscanf) - 5 tests
- [ ] Memory allocation (malloc/free) - 2 tests  
- [ ] Constructor/destructor support (atexit, constructor) - 2 tests
- [ ] Bit operations (ffs, etc.) - 1 test

### Medium Priority Issues (Extended Functionality)

- [ ] Math functions (floating point operations) - multiple tests
- [ ] Complex I/O (snprintf formatting) - 1 test
- [ ] Wide character support (wcstol) - 1 test

### Low Priority Issues (Advanced Features)

- [ ] Signal handling (abort, raise) - skipped, may not implement
- [ ] Complex numbers - skipped, may not implement  
- [ ] Atomic operations - skipped for bare metal
- [ ] Thread-local storage - skipped for bare metal

### Performance Improvements

- [ ] Enable -O1/-O2 optimization (currently using -O0)
- [ ] Reduce timeout tests (investigate slow execution)
- [ ] Optimize frequently-used operations

## Test Suite Goals

**Short-term goal**: Get core libc_testsuite tests passing (currently 5/10)  
**Medium-term goal**: Get all non-timeout tests passing  
**Long-term goal**: Optimize performance to reduce timeouts
