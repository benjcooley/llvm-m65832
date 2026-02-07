# M65832 Picolibc Testing - Quick Reference Card

## Essential Commands

### Running Tests

```bash
# Navigate to emulator directory first
cd /Users/benjamincooley/projects/m65832/emu

# List all available tests
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --list

# Run a single test
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py <test_name>

# Run a test suite
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=baremetal
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=libc_testsuite

# Run tests matching a pattern
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --filter='str*'

# Run all tests (WARNING: takes a long time)
python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py
```

### Manual Compilation and Testing

```bash
# Set up environment
export LLVM_BUILD=/Users/benjamincooley/projects/llvm-m65832/build-fast/bin
export SYSROOT=/Users/benjamincooley/projects/m65832-sysroot

# Compile C to object file
$LLVM_BUILD/clang -target m65832-elf -O0 -ffreestanding \
    -I$SYSROOT/include -c test.c -o test.o

# Link to executable
/Users/benjamincooley/projects/llvm-m65832/build/bin/ld.lld \
    -T$SYSROOT/lib/m65832.ld \
    $SYSROOT/lib/crt0.o test.o \
    -L$SYSROOT/lib -lc -lsys -lcompiler_rt \
    -o test.elf

# Run on emulator
./m65832emu -c 10000000 --stop-on-brk test.elf
```

## Debugging Commands

### Generate LLVM IR

```bash
$LLVM_BUILD/clang -target m65832-elf -O0 -ffreestanding \
    -I$SYSROOT/include -S -emit-llvm test.c -o test.ll
cat test.ll
```

### Generate Assembly

```bash
$LLVM_BUILD/clang -target m65832-elf -O0 -ffreestanding \
    -I$SYSROOT/include -S test.c -o test.s
cat test.s
```

### Instruction Selection Debug Output

```bash
# General debug output
$LLVM_BUILD/clang -target m65832-elf -O0 -ffreestanding \
    -I$SYSROOT/include -mllvm -debug -S test.c 2>&1 | tee debug.log

# Instruction selection only
$LLVM_BUILD/clang -target m65832-elf -O0 -ffreestanding \
    -I$SYSROOT/include -mllvm -debug-only=isel -S test.c 2>&1 | tee isel.log

# View SelectionDAG
$LLVM_BUILD/clang -target m65832-elf -O0 -ffreestanding \
    -I$SYSROOT/include -mllvm -debug-only=isel \
    -mllvm -view-dag-combine1-dags -S test.c 2>&1
```

### Using LLC Directly

```bash
# Generate IR first
$LLVM_BUILD/clang -target m65832-elf -O0 -S -emit-llvm test.c -o test.ll

# Compile IR to assembly with debug
$LLVM_BUILD/llc -march=m65832 -debug-only=isel test.ll -o test.s 2>&1
```

## Key File Locations

### Test Infrastructure
- **Test runner**: `/Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py`
- **Picolibc tests**: `/Users/benjamincooley/projects/picolibc-m65832/test/`
- **Custom M65832 tests**: `/Users/benjamincooley/projects/m65832/emu/c_tests/baremetal/picolibc/`

### Build System
- **Compiler**: `/Users/benjamincooley/projects/llvm-m65832/build-fast/bin/clang`
- **Linker**: `/Users/benjamincooley/projects/llvm-m65832/build/bin/ld.lld`
- **Emulator**: `/Users/benjamincooley/projects/m65832/emu/m65832emu`
- **Sysroot**: `/Users/benjamincooley/projects/m65832-sysroot`

### Backend Code
- **Target directory**: `/Users/benjamincooley/projects/llvm-m65832/llvm/lib/Target/M65832/`
- **Instruction definitions**: `M65832InstrInfo.td`
- **Instruction selection**: `M65832ISelDAGToDAG.cpp`
- **Lowering**: `M65832ISelLowering.cpp`
- **Register info**: `M65832RegisterInfo.td`
- **Calling convention**: `M65832CallingConv.td`

### Documentation
- **Test status**: `/Users/benjamincooley/projects/llvm-m65832/m65832-stdlib/PICOLIBC_TEST_STATUS.md`
- **Fix log**: `/Users/benjamincooley/projects/llvm-m65832/docs/fixes/`
- **Fix template**: `/Users/benjamincooley/projects/llvm-m65832/docs/fixes/FIX-TEMPLATE.md`

## Common Debug Flags

| Flag | Purpose |
|------|---------|
| `-mllvm -debug` | Enable all LLVM debug output |
| `-mllvm -debug-only=isel` | Instruction selection debug only |
| `-mllvm -debug-only=regalloc` | Register allocation debug |
| `-mllvm -print-after-all` | Print IR after each pass |
| `-mllvm -print-machineinstrs` | Print machine instructions |
| `-mllvm -view-dag-combine1-dags` | Visualize SelectionDAG |
| `-S -emit-llvm` | Generate LLVM IR (.ll file) |
| `-S` | Generate assembly (.s file) |

## Typical Workflow for Fixing a Test

1. **Identify failing test**
   ```bash
   python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=libc_testsuite
   ```

2. **Find test source**
   ```bash
   find /Users/benjamincooley/projects/picolibc-m65832/test -name "*strtol*"
   ```

3. **Create minimal test case**
   - Copy test, progressively comment out code
   - Isolate the specific failing operation

4. **Debug with LLVM**
   ```bash
   $LLVM_BUILD/clang -target m65832-elf -mllvm -debug-only=isel -S minimal.c 2>&1
   ```

5. **Identify the issue**
   - Check SelectionDAG output
   - Look for "Cannot select" errors
   - Identify which IR operation fails

6. **Fix the backend**
   - Add missing patterns in `M65832InstrInfo.td`
   - Or implement custom lowering in `M65832ISelLowering.cpp`

7. **Test the fix**
   ```bash
   python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py <test_name>
   ```

8. **Create comprehensive tests**
   - Add test to `/Users/benjamincooley/projects/m65832/emu/c_tests/baremetal/picolibc/`
   - Cover edge cases, different types, etc.

9. **Check for regressions**
   ```bash
   python3 /Users/benjamincooley/projects/picolibc-m65832/run_picolibc_gtest.py --suite=baremetal
   ```

10. **Document the fix**
    - Create file in `/Users/benjamincooley/projects/llvm-m65832/docs/fixes/`
    - Update `docs/fixes/README.md`
    - Commit with reference to fix document

## Common Error Patterns

### "Cannot select: mul i32"
**Cause**: Missing TableGen pattern  
**Fix**: Add to `M65832InstrInfo.td`:
```tablegen
def : Pat<(mul GR32:$src1, GR32:$src2), (MUL32rr GR32:$src1, GR32:$src2)>;
```

### "undefined symbol: __mulsi3"
**Cause**: Missing compiler runtime function  
**Fix**: Implement in libcompiler_rt or check linking

### Test times out (30s)
**Cause**: Infinite loop or very slow execution  
**Fix**: Debug with emulator, check generated assembly

### Test returns wrong value
**Cause**: Miscompilation or logic error  
**Fix**: Compare generated assembly with expected behavior

## Test Status Summary (2026-02-07)

- **Total tests**: 181
- **Passing**: 162 (all non-skipped tests)
- **Failed**: 0
- **Skipped**: 19 (expected - missing platform features)
- **Baremetal suite**: 29/29 PASSING
- **Fast regression test**: `--suite=baremetal` (~6 seconds)

## Getting Help

1. Check main documentation: `PICOLIBC_TEST_STATUS.md`
2. Search fix log: `docs/fixes/README.md`
3. Look at similar LLVM backends (ARM, RISC-V, Mips)
4. Use LLVM documentation: https://llvm.org/docs/
