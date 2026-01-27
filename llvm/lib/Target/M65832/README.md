# M65832 LLVM Backend

LLVM backend for the M65832 processor - a 32-bit evolution of the 6502/65816 architecture.

## Status

**Current Phase: Working C/C++ compiler (Phase A)**

### Working Features

| Feature | Status | Notes |
|---------|--------|-------|
| **Clang C compiler** | ✅ | `clang -target m65832-unknown-elf` |
| **Clang C++ compiler** | ✅ | `clang++ -target m65832-unknown-elf -fno-exceptions` |
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
| DWARF debug info | ✅ | CFI directives supported |
| **Hardware FPU** | ✅ | 16x64-bit regs, hard-float ABI |

### Floating Point Support

The M65832 backend uses **hardware FPU** with a hard-float ABI:

| Feature | Status |
|---------|--------|
| f32 operations (add/sub/mul/div/neg/abs/sqrt) | ✅ Hardware FPU |
| f64 operations (add/sub/mul/div/neg/abs/sqrt) | ✅ Hardware FPU |
| f32/f64 conversions (FCVT.DS, FCVT.SD) | ✅ Hardware FPU |
| Trigonometric (sin/cos/tan) | Library calls |
| Transcendental (exp/log/pow) | Library calls |

**Hard-float ABI:**
- Float arguments passed in F0-F7
- Float return values in F0
- F0-F13 are caller-saved
- F14-F15 are callee-saved

**Example generated code:**
```c
float fadd(float a, float b) { return a + b; }
```
Generates:
```asm
fadd:
    FADD.S F0, F1    ; F0 = F0 + F1
    RTS
```

### Known Limitations

- SELECT_CC (conditional move) uses placeholder expansion
- Some condition codes (GT, LE) use approximations
- Comparison falls back to LDA/CMP instead of CMPR_DP
- No hardware multiply/divide (uses libcalls)
- FP comparisons (FCMP) not yet fully integrated
- C++ exceptions not yet supported

## Building

```bash
# Configure with Clang (from llvm-project root)
cmake -S llvm -B build \
  -DLLVM_TARGETS_TO_BUILD="M65832" \
  -DLLVM_ENABLE_PROJECTS="clang" \
  -DCMAKE_BUILD_TYPE=Release

# Build clang and llc
make -C build -j$(nproc) clang llc

# Test C compilation
echo 'int add(int a, int b) { return a + b; }' > /tmp/test.c
build/bin/clang -target m65832-unknown-elf -S -O2 -o /tmp/test.s /tmp/test.c
cat /tmp/test.s
```

## Architecture Overview

### Register File

The M65832 has a 64-register window (R0-R63) accessed via Direct Page addressing:
- R0 = $00, R1 = $04, R2 = $08, ... R63 = $FC

**GPR Usage Convention:**
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

**FPU Registers (optional, when FPU feature enabled):**
| Registers | Width | Usage |
|-----------|-------|-------|
| F0-F7 | 64-bit | Arguments / Return values (caller-saved) |
| F8-F13 | 64-bit | Temporaries (caller-saved) |
| F14-F15 | 64-bit | Callee-saved |

Each FPU register holds either IEEE 754 binary64 (double) or binary32 (float, low 32 bits).

### Extended Instructions

The backend uses M65832 extended instructions ($02 prefix) with explicit size:

**Extended ALU ($02 $80-$97):**
```asm
ADC.L R0,R1          ; R0 = R0 + R1 + C (32-bit)
SBC.W R0,#$0042      ; R0 = R0 - 0x42 (16-bit)
LD.B  R0,R1          ; R0 = R1 (8-bit)
LD.L  R0,#$00001234  ; R0 = 0x1234 (32-bit)
```

**Barrel Shifter ($02 $98):**
```asm
SHL  R0,R1,#$03   ; R0 = R1 << 3
SHR  R0,R0,#$01   ; R0 = R0 >> 1 (logical)
SAR  R0,R0,#$04   ; R0 = R0 >> 4 (arithmetic)
```

**Extend Operations ($02 $99):**
```asm
SEXT8  R0,R1      ; R0 = sign_extend_8(R1)
CLZ    R0,R1      ; R0 = count_leading_zeros(R1)
```

**FPU Instructions ($02 $B0-$EF):**
```asm
; Load/Store (64-bit)
LDF F0, $20       ; F0 = [DP+$20] (direct page)
STF F5, $100      ; [B+$100] = F5 (bank-relative)

; Single-precision arithmetic (Fd = Fd op Fs)
FADD.S F0, F1     ; F0 = F0 + F1
FSUB.S F2, F3     ; F2 = F2 - F3
FMUL.S F0, F4     ; F0 = F0 * F4
FDIV.S F1, F2     ; F1 = F1 / F2
FNEG.S F0, F1     ; F0 = -F1
FABS.S F0, F1     ; F0 = |F1|
FSQRT.S F0, F1    ; F0 = sqrt(F1)

; Double-precision (same format, .D suffix)
FADD.D F0, F1     ; F0 = F0 + F1 (64-bit)
FMUL.D F2, F3     ; F2 = F2 * F3

; Conversions
FCVT.DS F0, F1    ; F0 = (double)F1 (extend)
FCVT.SD F0, F1    ; F0 = (float)F1 (truncate)
F2I.S F0          ; A = (int)F0
I2F.S F0          ; F0 = (float)A
```

### Assembly Syntax

Traditional 6502/65816 mnemonics use uppercase concatenated form:
```asm
LDA  $00         ; Load accumulator from DP
STA  $04         ; Store accumulator to DP
ADC  #$00000042  ; Add with carry immediate (32-bit default)
```

Extended ALU instructions use register notation and explicit sizes:
```asm
ADC.L R0,R1          ; Add registers (32-bit)
LD.W  R0,#$1234      ; Load 16-bit immediate
LD.B  R0,R1          ; Load 8-bit register
SHL   R0,R0,#$03     ; Shift left by constant

LDA  B+$1234         ; Bank-relative 16-bit address
LD.L R0,$A0001234    ; 32-bit absolute (extended ALU)
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
# Compile C to assembly
clang -target m65832-unknown-elf -S -O2 -o test.s test.c

# Assemble (using m65832 assembler)
../m65832/as/m65832as test.s -o test.bin

# Run on emulator
../m65832/emu/m65832emu test.bin
```

## Clang Target Support

The M65832 target is fully integrated into Clang:

```bash
# C compilation
clang -target m65832-unknown-elf -S -O2 -o output.s input.c

# C++ compilation (no exceptions)
clang++ -target m65832-unknown-elf -S -O2 -fno-exceptions -o output.s input.cpp

# Full pipeline to binary
clang -target m65832-unknown-elf -S -O2 -o output.s input.c
m65832as output.s -o output.bin
```

### Target-Specific Macros

When compiling for M65832, these macros are defined:
- `__m65832__`
- `__M65832__`
- `M65832`
- `__LITTLE_ENDIAN__`

## Next Steps

1. **Linker support** - Configure lld or external linker
2. **C runtime** - crt0.s, libcalls for mul/div
3. **Libc port** - Picolibc or newlib
4. **FP comparisons** - Complete FCMP integration for floating-point conditionals
5. **Exception handling** - C++ exception support

## License

Part of the LLVM Project, under Apache License v2.0 with LLVM Exceptions.
