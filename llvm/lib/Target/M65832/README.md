# M65832 LLVM Backend

LLVM backend for the M65832 processor - a 32-bit evolution of the 6502/65816 architecture.

## Status

**Current Phase: Working but sub-optimal compiler (Phase A)**

### Working Features

| Feature | Status | Notes |
|---------|--------|-------|
| Basic arithmetic (ADD, SUB) | ✅ | Uses extended register-targeted ALU |
| Logical ops (AND, OR, XOR) | ✅ | Extended instructions |
| Shifts (SHL, SHR, SAR) | ✅ | Hardware barrel shifter |
| Rotates (ROL, ROR) | ✅ | Hardware barrel shifter |
| Bit ops (CLZ, CTZ, POPCNT) | ✅ | Hardware support |
| Sign/zero extend | ✅ | SEXT8, SEXT16, ZEXT8, ZEXT16 |
| Function calls | ✅ | JSR/RTS |
| Stack frames | ✅ | Alloca, local variables |
| Global variables | ✅ | Load/store |
| Conditional branches | ✅ | BEQ, BNE, BMI, BPL, etc. |
| ELF object output | ✅ | EM_M65832 = 0x6583 |

### Known Limitations

- SELECT_CC (conditional move) uses placeholder expansion
- Some condition codes (GT, LE) use approximations
- Comparison falls back to LDA/CMP instead of CMPR_DP
- No hardware multiply/divide (uses libcalls)

## Building

```bash
# Configure (from llvm-project root)
cmake -G Ninja -S llvm -B build \
  -DLLVM_TARGETS_TO_BUILD="M65832" \
  -DCMAKE_BUILD_TYPE=Release

# Build llc
ninja -C build llc

# Test
echo 'define i32 @add(i32 %a, i32 %b) { %r = add i32 %a, %b ret i32 %r }' | \
  build/bin/llc -march=m65832 -o -
```

## Architecture Overview

### Register File

The M65832 has a 64-register window (R0-R63) accessed via Direct Page addressing:
- R0 = $00, R1 = $04, R2 = $08, ... R63 = $FC

**Register Usage Convention:**
| Registers | Usage |
|-----------|-------|
| R0-R7 | Arguments / Return values |
| R8-R23 | Caller-saved temporaries |
| R24-R27 | Kernel reserved |
| R28 | Global pointer |
| R29 | Frame pointer (if needed) |
| R30 | Link register / scratch |
| R31 | Reserved |
| R32-R55 | Callee-saved |
| R56-R63 | Reserved for future |

### Extended Instructions

The backend uses M65832 extended instructions ($02 prefix):

**Register-Targeted ALU ($02 $E8):**
```asm
ADD  R0,R1       ; R0 = R0 + R1
SUB  R0,##42     ; R0 = R0 - 42
LD   R0,##1234   ; R0 = 1234
```

**Barrel Shifter ($02 $E9):**
```asm
SHL  R0,R1,##3   ; R0 = R1 << 3
SHR  R0,R0,##1   ; R0 = R0 >> 1 (logical)
SAR  R0,R0,##4   ; R0 = R0 >> 4 (arithmetic)
```

**Extend Operations ($02 $EA):**
```asm
SEXT8  R0,R1     ; R0 = sign_extend_8(R1)
CLZ    R0,R1     ; R0 = count_leading_zeros(R1)
```

### Assembly Syntax

Traditional 6502/65816 mnemonics use uppercase concatenated form:
```asm
LDA  $00         ; Load accumulator from DP
STA  $04         ; Store accumulator to DP
ADC  #$42        ; Add with carry immediate
```

New extended instructions use register notation:
```asm
ADD  R0,R1       ; Add registers
LD   R0,##1234   ; Load immediate (## = 32-bit)
SHL  R0,R0,##3   ; Shift left by constant
```

## File Structure

```
M65832/
├── M65832.h                    # Target definitions
├── M65832.td                   # Main TableGen file
├── M65832RegisterInfo.td       # Register definitions
├── M65832InstrInfo.td          # Instruction definitions
├── M65832InstrFormats.td       # Instruction encoding formats
├── M65832CallingConv.td        # Calling convention
├── M65832ISelLowering.cpp/h    # Instruction selection lowering
├── M65832ISelDAGToDAG.cpp      # DAG to DAG pattern matching
├── M65832InstrInfo.cpp/h       # Instruction info & expansion
├── M65832RegisterInfo.cpp/h    # Register info & frame handling
├── M65832FrameLowering.cpp/h   # Stack frame management
├── M65832Subtarget.cpp/h       # Target features
├── M65832TargetMachine.cpp/h   # Target machine definition
├── M65832AsmPrinter.cpp        # Assembly output
├── M65832MachineFunctionInfo.h # Per-function info
└── MCTargetDesc/               # MC layer (encoding, ELF)
    ├── M65832MCCodeEmitter.cpp
    ├── M65832AsmBackend.cpp
    ├── M65832ELFObjectWriter.cpp
    └── ...
```

## Testing with Emulator

The M65832 emulator is in the sibling `m65832` repo:

```bash
# Compile LLVM IR to assembly
llc -march=m65832 test.ll -o test.s

# Assemble (using m65832 assembler)
../m65832/as/m65832as test.s -o test.bin

# Run on emulator
../m65832/emu/m65832emu test.bin
```

## Next Steps

1. **Build clang** - Enable `clang -target m65832`
2. **Linker support** - Configure lld or external linker
3. **C runtime** - crt0.s, libcalls for mul/div
4. **Libc port** - Picolibc or newlib

## License

Part of the LLVM Project, under Apache License v2.0 with LLVM Exceptions.
