//===-- M65832RegisterInfo.td - M65832 Register defs ------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Declarations that describe the M65832 register file.
//
// Register Allocation (64 registers + architectural):
//
//   R0-R7      Arguments / Return values      (8)  caller-saved
//   R8-R15     Temporaries                    (8)  caller-saved
//   R16-R23    Preserved                      (8)  callee-saved
//   R24-R27    Kernel reserved                (4)  reserved
//   R28        Global pointer (GP)            (1)  reserved
//   R29        Frame pointer (FP)             (1)  callee-saved
//   R30        Link register (optional)       (1)  caller-saved
//   R31        Reserved                       (1)  reserved
//   R32-R47    Additional temporaries        (16)  caller-saved
//   R48-R55    Additional preserved           (8)  callee-saved
//   R56-R63    Reserved for future/FP/TLS     (8)  reserved
//
//   A, X, Y    Accumulator, Index registers        caller-saved
//   SP         Stack pointer                       (special)
//   D, B, VBR  Base registers                      reserved
//   T          Temp (MUL high/DIV remainder)       reserved
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Register Definitions
//===----------------------------------------------------------------------===//

class M65832Reg<bits<7> Enc, string n, list<string> alt = []> : Register<n> {
  let HWEncoding{6-0} = Enc;
  let Namespace = "M65832";
  let AltNames = alt;
}

//===----------------------------------------------------------------------===//
// Register Window: R0-R63
// Accessed via Direct Page: R0 at $00, R1 at $04, ..., R63 at $FC
//===----------------------------------------------------------------------===//

// Arguments / Return values (caller-saved)
def R0  : M65832Reg<0,  "R0">,  DwarfRegNum<[0]>;
def R1  : M65832Reg<1,  "R1">,  DwarfRegNum<[1]>;
def R2  : M65832Reg<2,  "R2">,  DwarfRegNum<[2]>;
def R3  : M65832Reg<3,  "R3">,  DwarfRegNum<[3]>;
def R4  : M65832Reg<4,  "R4">,  DwarfRegNum<[4]>;
def R5  : M65832Reg<5,  "R5">,  DwarfRegNum<[5]>;
def R6  : M65832Reg<6,  "R6">,  DwarfRegNum<[6]>;
def R7  : M65832Reg<7,  "R7">,  DwarfRegNum<[7]>;

// Temporaries (caller-saved)
def R8  : M65832Reg<8,  "R8">,  DwarfRegNum<[8]>;
def R9  : M65832Reg<9,  "R9">,  DwarfRegNum<[9]>;
def R10 : M65832Reg<10, "R10">, DwarfRegNum<[10]>;
def R11 : M65832Reg<11, "R11">, DwarfRegNum<[11]>;
def R12 : M65832Reg<12, "R12">, DwarfRegNum<[12]>;
def R13 : M65832Reg<13, "R13">, DwarfRegNum<[13]>;
def R14 : M65832Reg<14, "R14">, DwarfRegNum<[14]>;
def R15 : M65832Reg<15, "R15">, DwarfRegNum<[15]>;

// Preserved (callee-saved)
def R16 : M65832Reg<16, "R16">, DwarfRegNum<[16]>;
def R17 : M65832Reg<17, "R17">, DwarfRegNum<[17]>;
def R18 : M65832Reg<18, "R18">, DwarfRegNum<[18]>;
def R19 : M65832Reg<19, "R19">, DwarfRegNum<[19]>;
def R20 : M65832Reg<20, "R20">, DwarfRegNum<[20]>;
def R21 : M65832Reg<21, "R21">, DwarfRegNum<[21]>;
def R22 : M65832Reg<22, "R22">, DwarfRegNum<[22]>;
def R23 : M65832Reg<23, "R23">, DwarfRegNum<[23]>;

// Kernel reserved
def R24 : M65832Reg<24, "R24">, DwarfRegNum<[24]>;
def R25 : M65832Reg<25, "R25">, DwarfRegNum<[25]>;
def R26 : M65832Reg<26, "R26">, DwarfRegNum<[26]>;
def R27 : M65832Reg<27, "R27">, DwarfRegNum<[27]>;

// Special purpose
def R28 : M65832Reg<28, "R28", ["gp"]>, DwarfRegNum<[28]>;  // Global pointer
def R29 : M65832Reg<29, "R29", ["fp"]>, DwarfRegNum<[29]>;  // Frame pointer
def R30 : M65832Reg<30, "R30", ["lr"]>, DwarfRegNum<[30]>;  // Link register
def R31 : M65832Reg<31, "R31">, DwarfRegNum<[31]>;          // Reserved

// Additional temporaries (caller-saved)
def R32 : M65832Reg<32, "R32">, DwarfRegNum<[32]>;
def R33 : M65832Reg<33, "R33">, DwarfRegNum<[33]>;
def R34 : M65832Reg<34, "R34">, DwarfRegNum<[34]>;
def R35 : M65832Reg<35, "R35">, DwarfRegNum<[35]>;
def R36 : M65832Reg<36, "R36">, DwarfRegNum<[36]>;
def R37 : M65832Reg<37, "R37">, DwarfRegNum<[37]>;
def R38 : M65832Reg<38, "R38">, DwarfRegNum<[38]>;
def R39 : M65832Reg<39, "R39">, DwarfRegNum<[39]>;
def R40 : M65832Reg<40, "R40">, DwarfRegNum<[40]>;
def R41 : M65832Reg<41, "R41">, DwarfRegNum<[41]>;
def R42 : M65832Reg<42, "R42">, DwarfRegNum<[42]>;
def R43 : M65832Reg<43, "R43">, DwarfRegNum<[43]>;
def R44 : M65832Reg<44, "R44">, DwarfRegNum<[44]>;
def R45 : M65832Reg<45, "R45">, DwarfRegNum<[45]>;
def R46 : M65832Reg<46, "R46">, DwarfRegNum<[46]>;
def R47 : M65832Reg<47, "R47">, DwarfRegNum<[47]>;

// Additional preserved (callee-saved)
def R48 : M65832Reg<48, "R48">, DwarfRegNum<[48]>;
def R49 : M65832Reg<49, "R49">, DwarfRegNum<[49]>;
def R50 : M65832Reg<50, "R50">, DwarfRegNum<[50]>;
def R51 : M65832Reg<51, "R51">, DwarfRegNum<[51]>;
def R52 : M65832Reg<52, "R52">, DwarfRegNum<[52]>;
def R53 : M65832Reg<53, "R53">, DwarfRegNum<[53]>;
def R54 : M65832Reg<54, "R54">, DwarfRegNum<[54]>;
def R55 : M65832Reg<55, "R55">, DwarfRegNum<[55]>;

// Reserved for future/FP/TLS
def R56 : M65832Reg<56, "R56">, DwarfRegNum<[56]>;
def R57 : M65832Reg<57, "R57">, DwarfRegNum<[57]>;
def R58 : M65832Reg<58, "R58">, DwarfRegNum<[58]>;
def R59 : M65832Reg<59, "R59">, DwarfRegNum<[59]>;
def R60 : M65832Reg<60, "R60">, DwarfRegNum<[60]>;
def R61 : M65832Reg<61, "R61">, DwarfRegNum<[61]>;
def R62 : M65832Reg<62, "R62">, DwarfRegNum<[62]>;
def R63 : M65832Reg<63, "R63">, DwarfRegNum<[63]>;

//===----------------------------------------------------------------------===//
// Architectural Registers
//===----------------------------------------------------------------------===//

def A   : M65832Reg<64, "a">,   DwarfRegNum<[64]>;   // Accumulator
def X   : M65832Reg<65, "x">,   DwarfRegNum<[65]>;   // Index X
def Y   : M65832Reg<66, "y">,   DwarfRegNum<[66]>;   // Index Y
def SP  : M65832Reg<67, "sp">,  DwarfRegNum<[67]>;   // Stack pointer
def D   : M65832Reg<68, "d">,   DwarfRegNum<[68]>;   // Direct page base
def B   : M65832Reg<69, "b">,   DwarfRegNum<[69]>;   // Absolute base
def VBR : M65832Reg<70, "vbr">, DwarfRegNum<[70]>;   // Virtual base register
def T   : M65832Reg<71, "t">,   DwarfRegNum<[71]>;   // Temp (MUL/DIV)
def SR  : M65832Reg<72, "sr">,  DwarfRegNum<[72]>;   // Status register

//===----------------------------------------------------------------------===//
// Register Classes
//===----------------------------------------------------------------------===//

// Primary allocatable GPRs - ordered by allocation preference
// (volatile first for better code, callee-saved later to minimize saves)
def GPR : RegisterClass<"M65832", [i32], 32,
  (add
    // Caller-saved: arguments then temporaries
    R0, R1, R2, R3, R4, R5, R6, R7,           // Arguments
    R8, R9, R10, R11, R12, R13, R14, R15,     // Temporaries
    R30,                                       // Link register
    R32, R33, R34, R35, R36, R37, R38, R39,   // Extended temporaries
    R40, R41, R42, R43, R44, R45, R46, R47,
    // Callee-saved
    R16, R17, R18, R19, R20, R21, R22, R23,   // Preserved
    R48, R49, R50, R51, R52, R53, R54, R55,   // Extended preserved
    R29                                        // Frame pointer (last resort)
  )>;

// Argument registers only
def GPRArg : RegisterClass<"M65832", [i32], 32,
  (add R0, R1, R2, R3, R4, R5, R6, R7)>;

// Callee-saved registers
def GPRCalleeSaved : RegisterClass<"M65832", [i32], 32,
  (add R16, R17, R18, R19, R20, R21, R22, R23,
       R48, R49, R50, R51, R52, R53, R54, R55, R29)>;

// Accumulator - for operations that must use A
def ACC : RegisterClass<"M65832", [i32], 32, (add A)> {
  let CopyCost = 2;
}

// Index registers
def IDXREG : RegisterClass<"M65832", [i32], 32, (add X, Y)> {
  let CopyCost = 2;
}

// X register specifically (for CAS expected value)
def XREG : RegisterClass<"M65832", [i32], 32, (add X)> {
  let CopyCost = 2;
}

// Combined A/X/Y - useful as scratch when needed
def AXY : RegisterClass<"M65832", [i32], 32, (add A, X, Y)> {
  let CopyCost = 2;
}

// Stack pointer
def SPREG : RegisterClass<"M65832", [i32], 32, (add SP)> {
  let isAllocatable = 0;
}

// Base registers (not allocatable)
def BaseRegs : RegisterClass<"M65832", [i32], 32, (add D, B, VBR)> {
  let isAllocatable = 0;
}

// Temp register for MUL/DIV
def TREG : RegisterClass<"M65832", [i32], 32, (add T)> {
  let isAllocatable = 0;
}

// Status register
def SREG : RegisterClass<"M65832", [i32], 32, (add SR)> {
  let isAllocatable = 0;
  let CopyCost = -1;
}

// Reserved registers (kernel, GP, reserved)
def GPRReserved : RegisterClass<"M65832", [i32], 32,
  (add R24, R25, R26, R27, R28, R31,
       R56, R57, R58, R59, R60, R61, R62, R63)> {
  let isAllocatable = 0;
}

//===----------------------------------------------------------------------===//
// FPU Registers (optional, when HasFPU=true)
//
// The M65832 FPU has sixteen 64-bit registers (F0-F15) with two-operand
// destructive operations: Fd = Fd op Fs
//
// Register allocation:
//   F0-F7   Arguments / return values (caller-saved)
//   F8-F13  Temporaries (caller-saved)
//   F14-F15 Callee-saved
//
// Each register can hold either:
//   - Double-precision (IEEE 754 binary64): full 64 bits
//   - Single-precision (IEEE 754 binary32): low 32 bits
//===----------------------------------------------------------------------===//

// FPU Register definitions
def F0  : M65832Reg<80,  "F0">,  DwarfRegNum<[80]>;
def F1  : M65832Reg<81,  "F1">,  DwarfRegNum<[81]>;
def F2  : M65832Reg<82,  "F2">,  DwarfRegNum<[82]>;
def F3  : M65832Reg<83,  "F3">,  DwarfRegNum<[83]>;
def F4  : M65832Reg<84,  "F4">,  DwarfRegNum<[84]>;
def F5  : M65832Reg<85,  "F5">,  DwarfRegNum<[85]>;
def F6  : M65832Reg<86,  "F6">,  DwarfRegNum<[86]>;
def F7  : M65832Reg<87,  "F7">,  DwarfRegNum<[87]>;
def F8  : M65832Reg<88,  "F8">,  DwarfRegNum<[88]>;
def F9  : M65832Reg<89,  "F9">,  DwarfRegNum<[89]>;
def F10 : M65832Reg<90,  "F10">, DwarfRegNum<[90]>;
def F11 : M65832Reg<91,  "F11">, DwarfRegNum<[91]>;
def F12 : M65832Reg<92,  "F12">, DwarfRegNum<[92]>;
def F13 : M65832Reg<93,  "F13">, DwarfRegNum<[93]>;
def F14 : M65832Reg<94,  "F14">, DwarfRegNum<[94]>;
def F15 : M65832Reg<95,  "F15">, DwarfRegNum<[95]>;

// FPU register class for 64-bit doubles - ordered by allocation preference
// (volatile first, callee-saved last)
def FPR64 : RegisterClass<"M65832", [f64], 64,
  (add
    // Caller-saved: arguments then temporaries
    F0, F1, F2, F3, F4, F5, F6, F7,   // Arguments / return
    F8, F9, F10, F11, F12, F13,       // Temporaries
    // Callee-saved
    F14, F15
  )> {
  let CopyCost = 2;  // FMOV is relatively cheap
}

// FPU register class for 32-bit floats (uses same registers, low 32 bits)
def FPR32 : RegisterClass<"M65832", [f32], 32,
  (add
    F0, F1, F2, F3, F4, F5, F6, F7,
    F8, F9, F10, F11, F12, F13,
    F14, F15
  )> {
  let CopyCost = 2;
}

// FPU argument registers only
def FPRArg : RegisterClass<"M65832", [f32, f64], 64,
  (add F0, F1, F2, F3, F4, F5, F6, F7)>;

// FPU callee-saved registers
def FPRCalleeSaved : RegisterClass<"M65832", [f32, f64], 64,
  (add F14, F15)>;
