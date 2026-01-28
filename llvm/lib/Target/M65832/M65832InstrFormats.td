//===-- M65832InstrFormats.td - M65832 Instruction Formats -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// M65832 has variable-length instructions (1-6 bytes).
// Encoding is handled by the custom MCCodeEmitter.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Instruction Format Superclass
//===----------------------------------------------------------------------===//

class M65832Inst<dag outs, dag ins, string asmstr, list<dag> pattern>
    : Instruction {
  let Namespace = "M65832";
  
  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = pattern;
  
  // Default size
  let Size = 1;
  
  // Store the opcode for the custom encoder
  bits<8> Opcode = 0;
}

// Pseudo instruction (no encoding)
class Pseudo<dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let isPseudo = 1;
  let isCodeGenOnly = 1;
}

//===----------------------------------------------------------------------===//
// Format Classes
//===----------------------------------------------------------------------===//

// F0: Implied / Accumulator (1 byte)
class F0<bits<8> opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 1;
  let Opcode = opcode;
}

// F1: Direct Page (2 bytes)
class F1<bits<8> opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 2;
  let Opcode = opcode;
}

// F2: Immediate 8-bit (2 bytes)
class F2<bits<8> opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 2;
  let Opcode = opcode;
}

// F3: Absolute 16-bit (3 bytes)
class F3<bits<8> opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 3;
  let Opcode = opcode;
}

// F4: Immediate 16-bit (3 bytes)
class F4<bits<8> opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 3;
  let Opcode = opcode;
}

// F5: Relative 8-bit (2 bytes)
class F5<bits<8> opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 2;
  let Opcode = opcode;
}

// F6: Relative 16-bit (3 bytes)
class F6<bits<8> opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 3;
  let Opcode = opcode;
}

// F7_Imp: Extended Implied ($02 prefix, 2 bytes total)
class F7_Imp<bits<8> ext_opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 2;
  let Opcode = ext_opcode;
}

// F7_DP: Extended Direct Page ($02 prefix, 3 bytes total)
class F7_DP<bits<8> ext_opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 3;
  let Opcode = ext_opcode;
}

// F7_Imm8: Extended Immediate 8-bit ($02 prefix, 3 bytes total)
class F7_Imm8<bits<8> ext_opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 3;
  let Opcode = ext_opcode;
}

// F7_Imm32: Extended Immediate 32-bit ($02 prefix, 6 bytes total)
class F7_Imm32<bits<8> ext_opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 6;
  let Opcode = ext_opcode;
}

// F8: Immediate 32-bit (5 bytes total)
class F8<bits<8> opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 5;
  let Opcode = opcode;
}

// FExt0: Extended instruction with no operands (2 bytes: 0x02 + subop)
class FExt0<bits<8> subop, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 2;
  let Opcode = subop;
}

// F9: Absolute 16-bit (3 bytes total)
class F9<bits<8> opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 3;
  let Opcode = opcode;
}

//===----------------------------------------------------------------------===//
// Extended Instruction Formats ($02 prefix)
//===----------------------------------------------------------------------===//

// FE8: Extended ALU ($02 $80-$97 [mode] [dest] [src...])
// Size varies: 4-8 bytes depending on addressing mode
class FE8_DP<bits<8> opmode, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 5;  // $02 $op mode dest src_dp
  let Opcode = opmode;
}

class FE8_IMM<bits<8> opmode, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 8;  // $02 $op mode dest imm32
  let Opcode = opmode;
}

// FE9: Barrel shifter ($02 $98 op|cnt dest src)
class FE9<bits<8> opcnt, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 5;  // $02 $98 op|cnt dest src
  let Opcode = opcnt;
}

// FEA: Extend operations ($02 $99 subop dest src)
class FEA<bits<8> subop, dag outs, dag ins, string asmstr, list<dag> pattern>
    : M65832Inst<outs, ins, asmstr, pattern> {
  let Size = 5;  // $02 $99 subop dest src
  let Opcode = subop;
}
