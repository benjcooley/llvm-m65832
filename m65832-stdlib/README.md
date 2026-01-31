# M65832 Testing and Development Documentation Index

This directory contains comprehensive documentation for testing and fixing the M65832 LLVM backend and picolibc port.

## Quick Start

**Running Tests:**
```bash
cd /Users/benjamincooley/projects/m65832/emu
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=baremetal
```

**For AI Agents:** Start with `PICOLIBC_TEST_STATUS.md` Section 0 for the complete workflow.

## Documentation Files

### 1. [PICOLIBC_TEST_STATUS.md](./PICOLIBC_TEST_STATUS.md) - **Main Document**

**Purpose**: Complete guide for testing and fixing M65832 backend issues

**Contents**:
- Test runner locations and usage
- Current test results and status
- **Section 0**: AI agent workflow (test-driven development)
  - How to identify failing tests
  - How to prioritize which tests to fix
  - How to run tests and validate fixes
  - How to check for regressions
- **Section 1**: Code bisection and isolation techniques
  - Binary search with commenting
  - Operation isolation
  - Minimal test case creation
  - Debug output strategies
- **Section 2**: LLVM instruction selection debugging
  - Generating IR and assembly
  - Extensive debug flag usage
  - SelectionDAG analysis
  - TableGen pattern checking
  - Common error patterns
- **Section 3**: Creating comprehensive test coverage
  - Test categories and templates
  - Test organization
  - Regression testing
- **Section 4**: Documentation standards
  - Where to place fix documentation
  - Fix template and examples
  - Code and test documentation

**When to use**: This is your primary reference for fixing any test failures.

### 2. [QUICK-REFERENCE.md](./QUICK-REFERENCE.md) - **Command Cheat Sheet**

**Purpose**: Quick lookup for commands and locations

**Contents**:
- Essential test commands
- Manual compilation commands
- All debugging commands
- Key file locations
- Common debug flags table
- Typical workflow summary
- Common error patterns with fixes

**When to use**: When you need to quickly look up a command or file location.

### 3. [../docs/fixes/README.md](../docs/fixes/README.md) - **Fix Log Index**

**Purpose**: Central index of all fixes applied to the M65832 backend

**Contents**:
- Table of all documented fixes
- Common issue patterns
- Progress tracking
- Statistics and goals
- How to use the fix log

**When to use**: 
- Before fixing: Check if similar issue was already fixed
- After fixing: Add your fix to the index

### 4. [../docs/fixes/FIX-TEMPLATE.md](../docs/fixes/FIX-TEMPLATE.md) - **Fix Documentation Template**

**Purpose**: Standard template for documenting fixes

**Contents**:
- Problem description sections
- Investigation methodology
- Solution explanation
- Testing results
- Related issues
- Complete checklist

**When to use**: When documenting a fix you've implemented.

## Test Infrastructure

### Test Runner
- **Location**: `/Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py`
- **Language**: Python 3
- **Purpose**: Compiles and runs picolibc tests on M65832 emulator
- **Output Format**: Google Test (gtest) style

### Test Sources

1. **Picolibc Official Tests**
   - Location: `/Users/benjamincooley/projects/picolibc-m65832/test/`
   - Categories: libc-testsuite, test-string, test-stdio, test-math, etc.
   - Status: Mixed (some pass, some fail, some skip)

2. **Custom M65832 Tests**
   - Location: `/Users/benjamincooley/projects/m65832/emu/c_tests/baremetal/picolibc/`
   - Status: 27/27 baremetal tests PASSING ✓
   - Purpose: Core functionality validation

### Build System

- **Compiler**: `/Users/benjamincooley/projects/llvm-m65832/build-fast/bin/clang`
- **Linker**: `/Users/benjamincooley/projects/llvm-m65832/build/bin/ld.lld`
- **Emulator**: `/Users/benjamincooley/projects/m65832/emu/m65832emu`
- **Sysroot**: `/Users/benjamincooley/projects/m65832-sysroot`
  - Contains: picolibc headers, libraries, crt0, compiler runtime

## Current Status

**Test Results (as of 2026-01-31)**:
- Total available: 179 tests across 9 suites
- Baremetal suite: **27/27 PASSING (100%)** ✓
- Core functionality: **Working** (strings, stdlib, ctype)
- Remaining issues: ~15-20 tests requiring fixes
- Overall pass rate: ~15%, but 100% of core functionality

**What's Working**:
- ✅ All basic string operations (strlen, strcmp, strcpy, memcpy, memset, etc.)
- ✅ All stdlib basics (abs, atoi)
- ✅ All ctype functions (isalpha, isdigit, tolower, toupper)
- ✅ Function calls, branches, basic arithmetic

**What Needs Fixing**:
- ❌ Advanced string conversion (strtol, strtod, sscanf)
- ❌ Memory allocation (malloc/free - timing out)
- ❌ Some bit operations (ffs - timing out)
- ❌ Constructor/destructor support
- ⏭️ Advanced features (signals, complex numbers, atomics) - may not implement

## Workflow Summary

For a failing test, follow this process:

1. **Identify** → Run tests, find failures
2. **Locate** → Find test source code
3. **Isolate** → Create minimal failing test case (Section 1)
4. **Debug** → Use LLVM debug tools to find root cause (Section 2)
5. **Fix** → Implement the fix in backend or library
6. **Test** → Run the test again, verify it passes
7. **Expand** → Create comprehensive tests (Section 3)
8. **Validate** → Run regression tests
9. **Document** → Create fix documentation (Section 4)
10. **Commit** → Git commit with reference to fix doc

## For AI Agents

**Start Here**: Read `PICOLIBC_TEST_STATUS.md` Section 0 for the complete AI agent workflow.

**Key Points**:
1. Always start by running tests to see current status
2. Prioritize simple, high-impact tests first
3. Create minimal test cases to isolate issues
4. Use LLVM's extensive debug output
5. Always check for regressions after a fix
6. Document everything
7. Run the baremetal suite before committing (fast, comprehensive)

**Essential Command**:
```bash
cd /Users/benjamincooley/projects/m65832/emu
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=baremetal
```
This runs 27 core tests in ~6 seconds and validates basic functionality.

## For Human Developers

**Quick Commands**:
- List tests: `python3 run_picolibc_gtest.py --list`
- Run one test: `python3 run_picolibc_gtest.py <test_name>`
- Run fast suite: `python3 run_picolibc_gtest.py --suite=baremetal`

**Documentation Flow**:
1. Read QUICK-REFERENCE.md for commands
2. Follow PICOLIBC_TEST_STATUS.md for detailed procedures
3. Check docs/fixes/README.md for similar issues
4. Use docs/fixes/FIX-TEMPLATE.md when documenting

## Backend Code Locations

All backend code is in: `/Users/benjamincooley/projects/llvm-m65832/llvm/lib/Target/M65832/`

**Key Files**:
- `M65832InstrInfo.td` - Instruction definitions and patterns
- `M65832ISelDAGToDAG.cpp` - Custom instruction selection
- `M65832ISelLowering.cpp` - Operation lowering
- `M65832RegisterInfo.td` - Register definitions
- `M65832CallingConv.td` - Calling convention

## Getting Help

1. **Check documentation**: Start with PICOLIBC_TEST_STATUS.md
2. **Search fix log**: docs/fixes/README.md for similar issues
3. **LLVM docs**: https://llvm.org/docs/
4. **Similar backends**: Look at ARM, RISC-V, Mips implementations
5. **Debug output**: Use `-mllvm -debug-only=isel` (see Section 2)

## Contributing

When you fix an issue:
1. Create a fix document using FIX-TEMPLATE.md
2. Add entry to docs/fixes/README.md
3. Create comprehensive tests
4. Run regression tests
5. Commit with descriptive message

## Goals

**Short-term**: Fix libc_testsuite tests (currently 5/10 passing)
**Medium-term**: Fix all non-timeout tests
**Long-term**: Optimize to reduce timeouts, enable higher optimization levels

## License and Attribution

This documentation is part of the M65832 LLVM backend project.
Based on picolibc (https://github.com/picolibc/picolibc).
