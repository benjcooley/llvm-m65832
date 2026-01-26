//===-- M65832InstrInfo.td - M65832 Instruction defs -------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file describes M65832 instructions in TableGen format.
//
// The M65832 is a memory-to-memory architecture (6502 heritage).
// Most ALU operations flow through the accumulator (A):
//   1. LDA $xx     ; Load register Rn into A (where $xx = n*4)
//   2. <OP> $yy    ; Perform operation with Rm
//   3. STA $zz     ; Store result to Rk
//
// With R flag set, Direct Page addresses map to registers:
//   $00=R0, $04=R1, $08=R2, ..., $FC=R63
//
//===----------------------------------------------------------------------===//

include "M65832InstrFormats.td"

//===----------------------------------------------------------------------===//
// Type Profiles
//===----------------------------------------------------------------------===//

def SDT_M65832Call      : SDTypeProfile<0, -1, [SDTCisVT<0, i32>]>;
def SDT_M65832CallSeqStart : SDCallSeqStart<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;
def SDT_M65832CallSeqEnd   : SDCallSeqEnd<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;
def SDT_M65832Wrapper   : SDTypeProfile<1, 1, [SDTCisSameAs<0, 1>, SDTCisPtrTy<0>]>;
def SDT_M65832Cmp       : SDTypeProfile<0, 2, [SDTCisSameAs<0, 1>]>;
def SDT_M65832BrCC      : SDTypeProfile<0, 2, [SDTCisVT<0, OtherVT>, SDTCisVT<1, i32>]>;
def SDT_M65832SelectCC  : SDTypeProfile<1, 3, [SDTCisSameAs<0, 1>, SDTCisSameAs<1, 2>, SDTCisVT<3, i32>]>;

//===----------------------------------------------------------------------===//
// M65832 Specific Nodes
//===----------------------------------------------------------------------===//

def M65832retflag : SDNode<"M65832ISD::RET_FLAG", SDTNone,
                           [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

def M65832call    : SDNode<"M65832ISD::CALL", SDT_M65832Call,
                           [SDNPHasChain, SDNPOutGlue, SDNPOptInGlue, SDNPVariadic]>;

def M65832callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_M65832CallSeqStart,
                                 [SDNPHasChain, SDNPOutGlue]>;

def M65832callseq_end   : SDNode<"ISD::CALLSEQ_END", SDT_M65832CallSeqEnd,
                                 [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

def M65832wrapper : SDNode<"M65832ISD::WRAPPER", SDT_M65832Wrapper>;

def M65832cmp     : SDNode<"M65832ISD::CMP", SDT_M65832Cmp, [SDNPOutGlue]>;

def M65832brcc    : SDNode<"M65832ISD::BR_CC", SDT_M65832BrCC,
                           [SDNPHasChain, SDNPInGlue]>;

def M65832selectcc : SDNode<"M65832ISD::SELECT_CC", SDT_M65832SelectCC,
                            [SDNPInGlue]>;

//===----------------------------------------------------------------------===//
// Operand Definitions
//===----------------------------------------------------------------------===//

// Direct Page operand (register number * 4)
def DPOp : Operand<i32> {
  let PrintMethod = "printDPOperand";
  let EncoderMethod = "getDPOpValue";
  let DecoderMethod = "decodeDPOperand";
}

// Memory operand (for absolute addressing)
def MemOp : Operand<i32> {
  let PrintMethod = "printMemOperand";
  let MIOperandInfo = (ops GPR, i32imm);
}

// Bank-relative absolute address (B+$xxxx)
def BankRelOp : Operand<i32> {
  let PrintMethod = "printBankRelAddr";
}

// Immediate operands
def imm8  : Operand<i32>, ImmLeaf<i32, [{return isUInt<8>(Imm);}]>;
def imm16 : Operand<i32>, ImmLeaf<i32, [{return isUInt<16>(Imm);}]>;
// imm32 is just an alias for i32imm in 32-bit mode

// Branch target
def brtarget : Operand<OtherVT> {
  let PrintMethod = "printBranchTarget";
  let EncoderMethod = "getBranchTargetOpValue";
}

// Call target (for JSR)
def calltarget : Operand<i32> {
  let PrintMethod = "printBankRelAddr";
  let EncoderMethod = "getCallTargetOpValue";
}

// Condition code operand
def CCOp : Operand<i32> {
  let PrintMethod = "printCondCode";
}

// Address mode: base + offset
def ADDRri : ComplexPattern<i32, 2, "selectAddr", [frameindex]>;

// Wrapper pattern match
def M65832Wrapper : PatFrag<(ops node:$in), (M65832wrapper node:$in)>;

//===----------------------------------------------------------------------===//
// Pseudo Instructions (expanded during code generation)
//===----------------------------------------------------------------------===//

let Defs = [SP], Uses = [SP], hasSideEffects = 1, isCodeGenOnly = 1 in {
  def ADJCALLSTACKDOWN : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                                "# ADJCALLSTACKDOWN $amt1, $amt2",
                                [(M65832callseq_start timm:$amt1, timm:$amt2)]>;
  def ADJCALLSTACKUP   : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                                "# ADJCALLSTACKUP $amt1, $amt2",
                                [(M65832callseq_end timm:$amt1, timm:$amt2)]>;
}

// Select pseudo - expanded to branch sequence
let usesCustomInserter = 1, Uses = [SR] in {
  def SELECT : Pseudo<(outs GPR:$dst), (ins GPR:$src1, GPR:$src2, i32imm:$cc),
                      "# SELECT $dst, $src1, $src2, $cc",
                      []>;
}

//===----------------------------------------------------------------------===//
// Compare and Branch Pseudo Instructions
//===----------------------------------------------------------------------===//

// Compare two GPRs: LDA src1; CMP src2
let isCodeGenOnly = 1, Defs = [A, SR] in {
  def CMP_GPR : Pseudo<(outs), (ins GPR:$lhs, GPR:$rhs),
                       "# cmp $lhs, $rhs",
                       [(M65832cmp GPR:$lhs, GPR:$rhs)]>;

  def CMP_GPR_IMM : Pseudo<(outs), (ins GPR:$lhs, i32imm:$rhs),
                           "# cmp $lhs, $rhs",
                           [(M65832cmp GPR:$lhs, imm:$rhs)]>;
}

// Note: BR_CC_PSEUDO is defined later and handles M65832brcc pattern

//===----------------------------------------------------------------------===//
// Move / Copy Instructions
// Since M65832 is accumulator-based, we use LDA/STA sequences
//===----------------------------------------------------------------------===//

// Copy between GPRs (pseudo, expands to LDA $src; STA $dst)
let isCodeGenOnly = 1 in
def COPY_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src),
                      "# copy $dst, $src", []>;

// Load immediate into GPR (legacy - now using LDR_IMM)
// Uses: LDA #imm; STA $dp
def LI : Pseudo<(outs GPR:$dst), (ins i32imm:$imm),
                "# li $dst, $imm",
                []>;  // Pattern removed - use LDR_IMM

// Load effective address from frame index
// Computes: dst = FrameReg + offset
// The frame index and offset operands are replaced by eliminateFrameIndex
def LEA_FI : Pseudo<(outs GPR:$dst), (ins i32imm:$fi, i32imm:$offset),
                    "# lea_fi $dst, $fi, $offset",
                    []>;

// Load global/external address into GPR
def LA : Pseudo<(outs GPR:$dst), (ins i32imm:$addr),
                "# la $dst, $addr",
                [(set GPR:$dst, (M65832Wrapper tglobaladdr:$addr))]>;

def LA_EXT : Pseudo<(outs GPR:$dst), (ins i32imm:$addr),
                    "# la.ext $dst, $addr",
                    [(set GPR:$dst, (M65832Wrapper texternalsym:$addr))]>;

def LA_BA : Pseudo<(outs GPR:$dst), (ins i32imm:$addr),
                   "# la.ba $dst, $addr",
                   [(set GPR:$dst, (M65832Wrapper tblockaddress:$addr))]>;

//===----------------------------------------------------------------------===//
// ALU Pseudo Instructions (GPR to GPR via accumulator) - LEGACY
// These are expanded in M65832InstrInfo::expandPostRAPseudo
// NOTE: Patterns removed - prefer new extended instructions (ADDR_DP, etc.)
//===----------------------------------------------------------------------===//

let isCodeGenOnly = 1, Defs = [A] in {
  // ADD: dst = src1 + src2 (legacy 3-operand form)
  // Expands to: LDA src1; CLC; ADC src2; STA dst
  def ADD_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src1, GPR:$src2),
                       "# add $dst, $src1, $src2",
                       []>;  // Pattern removed - use ADDR_DP

  // SUB: dst = src1 - src2 (legacy 3-operand form)
  // Expands to: LDA src1; SEC; SBC src2; STA dst
  def SUB_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src1, GPR:$src2),
                       "# sub $dst, $src1, $src2",
                       []>;  // Pattern removed - use SUBR_DP

  // AND: dst = src1 & src2 (legacy 3-operand form)
  // Expands to: LDA src1; AND src2; STA dst
  def AND_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src1, GPR:$src2),
                       "# and $dst, $src1, $src2",
                       []>;  // Pattern removed - use ANDR_DP

  // OR: dst = src1 | src2 (legacy 3-operand form)
  // Expands to: LDA src1; ORA src2; STA dst
  def ORA_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src1, GPR:$src2),
                       "# ora $dst, $src1, $src2",
                       []>;  // Pattern removed - use ORAR_DP

  // XOR: dst = src1 ^ src2 (legacy 3-operand form)
  // Expands to: LDA src1; EOR src2; STA dst
  def EOR_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src1, GPR:$src2),
                       "# eor $dst, $src1, $src2",
                       []>;  // Pattern removed - use EORR_DP

  // ADD immediate (legacy) - patterns removed, use ADDR_IMM
  def ADDI_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src, i32imm:$imm),
                        "# addi $dst, $src, $imm",
                        []>;

  // SUB immediate (legacy) - patterns removed, use SUBR_IMM
  def SUBI_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src, i32imm:$imm),
                        "# subi $dst, $src, $imm",
                        []>;

  // AND immediate (legacy) - patterns removed, use ANDR_IMM
  def ANDI_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src, i32imm:$imm),
                        "# andi $dst, $src, $imm",
                        []>;

  // OR immediate (legacy) - patterns removed, use ORAR_IMM
  def ORI_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src, i32imm:$imm),
                       "# ori $dst, $src, $imm",
                       []>;

  // XOR immediate (legacy) - patterns removed, use EORR_IMM
  def XORI_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src, i32imm:$imm),
                        "# xori $dst, $src, $imm",
                        []>;
}

//===----------------------------------------------------------------------===//
// Optimized ALU Patterns
// These patterns match common operations and generate more efficient code
//===----------------------------------------------------------------------===//

// INC in place: dst = dst + 1 -> INC $dp
let isCodeGenOnly = 1, Defs = [SR] in {
  def INC_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src),
                       "# inc $dst",
                       []> {
    let Constraints = "$src = $dst";
  }
}

// DEC in place: dst = dst - 1 -> DEC $dp  
let isCodeGenOnly = 1, Defs = [SR] in {
  def DEC_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src),
                       "# dec $dst",
                       []> {
    let Constraints = "$src = $dst";
  }
}

// Store zero: Rn = 0 -> STZ $dp
let isCodeGenOnly = 1, Defs = [SR] in {
  def STZ_GPR : Pseudo<(outs GPR:$dst), (ins),
                       "# stz $dst",
                       [(set GPR:$dst, 0)]>;
}

// Double via ASL: dst = src * 2 -> LDA src; ASL A; STA dst
// (This is already handled by SHL with shift=1, but explicit is clearer)

// Negate: dst = 0 - src -> SEC; LDA #0; SBC src; STA dst
let isCodeGenOnly = 1, Defs = [A, SR] in {
  def NEG_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src),
                       "# neg $dst, $src",
                       [(set GPR:$dst, (sub 0, GPR:$src))]>;
}

// Increment by 1: Use ADDI pattern but optimize to INC when possible
def : Pat<(add GPR:$src, 1), (INC_GPR GPR:$src)>;
def : Pat<(add 1, GPR:$src), (INC_GPR GPR:$src)>;

// Decrement by 1
def : Pat<(add GPR:$src, -1), (DEC_GPR GPR:$src)>;
def : Pat<(sub GPR:$src, 1), (DEC_GPR GPR:$src)>;

//===----------------------------------------------------------------------===//
// Direct Memory Shift Operations (legacy - now using barrel shifter)
//===----------------------------------------------------------------------===//

// ASL on Direct Page: mem <<= 1 (legacy, kept for compatibility)
let isCodeGenOnly = 1, Defs = [SR] in {
  def ASL_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src),
                       "# asl_dp $dst",
                       []> {
    let Constraints = "$src = $dst";
  }
}

// LSR on Direct Page: mem >>= 1 (legacy, kept for compatibility)
let isCodeGenOnly = 1, Defs = [SR] in {
  def LSR_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src),
                       "# lsr_dp $dst",
                       []> {
    let Constraints = "$src = $dst";
  }
}

// NOTE: shift-by-1 now uses SHLR/SHRR (barrel shifter) via the general patterns

//===----------------------------------------------------------------------===//
// Extended ALU Instructions ($02 $80-$97)
// These bypass the accumulator for direct register-to-register operations
//===----------------------------------------------------------------------===//

// Op codes: LD=$80, ST=$81, ADC=$82, SBC=$83, AND=$84, ORA=$85, EOR=$86, CMP=$87
// Mode: size=long, target=Rn, addr_mode=dp or imm

let Defs = [SR] in {
  // ADD (via ADC): dest = dest + src
  def ADDR_DP : FE8_DP<0x82, (outs GPR:$dst), (ins GPR:$src1, GPR:$src2),
                       "ADC.L\t$dst,$src2",
                       [(set GPR:$dst, (add GPR:$src1, GPR:$src2))]> {
    let Constraints = "$src1 = $dst";
  }

  // SUB (via SBC): dest = dest - src
  def SUBR_DP : FE8_DP<0x83, (outs GPR:$dst), (ins GPR:$src1, GPR:$src2),
                       "SBC.L\t$dst,$src2",
                       [(set GPR:$dst, (sub GPR:$src1, GPR:$src2))]> {
    let Constraints = "$src1 = $dst";
  }

  // AND: dest = dest & src
  def ANDR_DP : FE8_DP<0x84, (outs GPR:$dst), (ins GPR:$src1, GPR:$src2),
                       "AND.L\t$dst,$src2",
                       [(set GPR:$dst, (and GPR:$src1, GPR:$src2))]> {
    let Constraints = "$src1 = $dst";
  }

  // ORA: dest = dest | src
  def ORAR_DP : FE8_DP<0x85, (outs GPR:$dst), (ins GPR:$src1, GPR:$src2),
                       "ORA.L\t$dst,$src2",
                       [(set GPR:$dst, (or GPR:$src1, GPR:$src2))]> {
    let Constraints = "$src1 = $dst";
  }

  // EOR: dest = dest ^ src
  def EORR_DP : FE8_DP<0x86, (outs GPR:$dst), (ins GPR:$src1, GPR:$src2),
                       "EOR.L\t$dst,$src2",
                       [(set GPR:$dst, (xor GPR:$src1, GPR:$src2))]> {
    let Constraints = "$src1 = $dst";
  }

  // CMP: flags = dest - src (no writeback)
  def CMPR_DP : FE8_DP<0x87, (outs), (ins GPR:$lhs, GPR:$rhs),
                       "CMP.L\t$lhs,$rhs",
                       [(M65832cmp GPR:$lhs, GPR:$rhs)]>;

  // MOV (via LD): dest = src
  def MOVR_DP : FE8_DP<0x80, (outs GPR:$dst), (ins GPR:$src),
                       "LD.L\t$dst,$src",
                       []>;

  // Immediate forms (Mode=$2)
  def ADDR_IMM : FE8_IMM<0x82, (outs GPR:$dst), (ins GPR:$src, i32imm:$imm),
                         "ADC.L\t$dst,#$imm",
                         [(set GPR:$dst, (add GPR:$src, imm:$imm))]> {
    let Constraints = "$src = $dst";
  }

  def SUBR_IMM : FE8_IMM<0x83, (outs GPR:$dst), (ins GPR:$src, i32imm:$imm),
                         "SBC.L\t$dst,#$imm",
                         [(set GPR:$dst, (sub GPR:$src, imm:$imm))]> {
    let Constraints = "$src = $dst";
  }

  def ANDR_IMM : FE8_IMM<0x84, (outs GPR:$dst), (ins GPR:$src, i32imm:$imm),
                         "AND.L\t$dst,#$imm",
                         [(set GPR:$dst, (and GPR:$src, imm:$imm))]> {
    let Constraints = "$src = $dst";
  }

  def ORAR_IMM : FE8_IMM<0x85, (outs GPR:$dst), (ins GPR:$src, i32imm:$imm),
                         "ORA.L\t$dst,#$imm",
                         [(set GPR:$dst, (or GPR:$src, imm:$imm))]> {
    let Constraints = "$src = $dst";
  }

  def EORR_IMM : FE8_IMM<0x86, (outs GPR:$dst), (ins GPR:$src, i32imm:$imm),
                         "EOR.L\t$dst,#$imm",
                         [(set GPR:$dst, (xor GPR:$src, imm:$imm))]> {
    let Constraints = "$src = $dst";
  }

  def CMPR_IMM : FE8_IMM<0x87, (outs), (ins GPR:$lhs, i32imm:$rhs),
                         "CMP.L\t$lhs,#$rhs",
                         [(M65832cmp GPR:$lhs, imm:$rhs)]>;

  // Load immediate into register
  def LDR_IMM : FE8_IMM<0x80, (outs GPR:$dst), (ins i32imm:$imm),
                        "LD.L\t$dst,#$imm",
                        [(set GPR:$dst, imm:$imm)]>;
}

//===----------------------------------------------------------------------===//
// Barrel Shifter Instructions ($02 $98)
// Single-cycle shifts with constant or variable amount
//===----------------------------------------------------------------------===//

// Op: 0=SHL, 1=SHR, 2=SAR, 3=ROL, 4=ROR
// Encoding: op(3 bits) | count(5 bits), count=$1F means shift by A

let Defs = [SR] in {
  // SHL with constant: op=0, cnt=imm -> (0<<5)|cnt
  def SHLR : FE9<0x00, (outs GPR:$dst), (ins GPR:$src, i32imm:$cnt),
                 "SHL\t$dst,$src,#$cnt",
                 [(set GPR:$dst, (shl GPR:$src, (i32 imm:$cnt)))]>;

  // SHR with constant: op=1 -> (1<<5)|cnt = $20|cnt
  def SHRR : FE9<0x20, (outs GPR:$dst), (ins GPR:$src, i32imm:$cnt),
                 "SHR\t$dst,$src,#$cnt",
                 [(set GPR:$dst, (srl GPR:$src, (i32 imm:$cnt)))]>;

  // SAR with constant: op=2 -> (2<<5)|cnt = $40|cnt
  def SARR : FE9<0x40, (outs GPR:$dst), (ins GPR:$src, i32imm:$cnt),
                 "SAR\t$dst,$src,#$cnt",
                 [(set GPR:$dst, (sra GPR:$src, (i32 imm:$cnt)))]>;

  // ROL with constant: op=3 -> (3<<5)|cnt = $60|cnt
  def ROLR : FE9<0x60, (outs GPR:$dst), (ins GPR:$src, i32imm:$cnt),
                 "ROL\t$dst,$src,#$cnt",
                 [(set GPR:$dst, (rotl GPR:$src, (i32 imm:$cnt)))]>;

  // ROR with constant: op=4 -> (4<<5)|cnt = $80|cnt
  def RORR : FE9<0x80, (outs GPR:$dst), (ins GPR:$src, i32imm:$cnt),
                 "ROR\t$dst,$src,#$cnt",
                 [(set GPR:$dst, (rotr GPR:$src, (i32 imm:$cnt)))]>;

  // Variable shifts (by A register): cnt=$1F
  def SHLR_VAR : FE9<0x1F, (outs GPR:$dst), (ins GPR:$src, ACC:$amt),
                     "SHL\t$dst,$src,A",
                     [(set GPR:$dst, (shl GPR:$src, ACC:$amt))]>;

  def SHRR_VAR : FE9<0x3F, (outs GPR:$dst), (ins GPR:$src, ACC:$amt),
                     "SHR\t$dst,$src,A",
                     [(set GPR:$dst, (srl GPR:$src, ACC:$amt))]>;

  def SARR_VAR : FE9<0x5F, (outs GPR:$dst), (ins GPR:$src, ACC:$amt),
                     "SAR\t$dst,$src,A",
                     [(set GPR:$dst, (sra GPR:$src, ACC:$amt))]>;
}

//===----------------------------------------------------------------------===//
// Extend Instructions ($02 $99)
//===----------------------------------------------------------------------===//

let Defs = [SR] in {
  // Sign extend 8-bit to 32-bit: subop=$00
  def SEXT8 : FEA<0x00, (outs GPR:$dst), (ins GPR:$src),
                  "SEXT8\t$dst,$src",
                  [(set GPR:$dst, (sext_inreg GPR:$src, i8))]>;

  // Sign extend 16-bit to 32-bit: subop=$01
  def SEXT16 : FEA<0x01, (outs GPR:$dst), (ins GPR:$src),
                   "SEXT16\t$dst,$src",
                   [(set GPR:$dst, (sext_inreg GPR:$src, i16))]>;

  // Zero extend 8-bit: subop=$02
  def ZEXT8 : FEA<0x02, (outs GPR:$dst), (ins GPR:$src),
                  "ZEXT8\t$dst,$src",
                  [(set GPR:$dst, (and GPR:$src, 0xFF))]>;

  // Zero extend 16-bit: subop=$03
  def ZEXT16 : FEA<0x03, (outs GPR:$dst), (ins GPR:$src),
                   "ZEXT16\t$dst,$src",
                   [(set GPR:$dst, (and GPR:$src, 0xFFFF))]>;

  // Count leading zeros: subop=$04
  def CLZ : FEA<0x04, (outs GPR:$dst), (ins GPR:$src),
                "CLZ\t$dst,$src",
                [(set GPR:$dst, (ctlz GPR:$src))]>;

  // Count trailing zeros: subop=$05
  def CTZ : FEA<0x05, (outs GPR:$dst), (ins GPR:$src),
                "CTZ\t$dst,$src",
                [(set GPR:$dst, (cttz GPR:$src))]>;

  // Population count: subop=$06
  def POPCNT : FEA<0x06, (outs GPR:$dst), (ins GPR:$src),
                   "POPCNT\t$dst,$src",
                   [(set GPR:$dst, (ctpop GPR:$src))]>;
}

//===----------------------------------------------------------------------===//
// Shift Pseudo Instructions (legacy - now using barrel shifter)
//===----------------------------------------------------------------------===//

let isCodeGenOnly = 1, Defs = [A, SR] in {
  // These are now fallbacks, prefer the new SHLR/SHRR/SARR instructions
  // SHL: dst = src << amt (constant)
  def SHL_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src, i32imm:$amt),
                       "# shl $dst, $src, $amt",
                       []>;  // Pattern removed - use SHLR

  // SRL: dst = src >> amt (logical, constant)
  def SRL_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src, i32imm:$amt),
                       "# srl $dst, $src, $amt",
                       []>;  // Pattern removed - use SHRR

  // SRA: dst = src >> amt (arithmetic, constant)
  def SRA_GPR : Pseudo<(outs GPR:$dst), (ins GPR:$src, i32imm:$amt),
                       "# sra $dst, $src, $amt",
                       []>;  // Pattern removed - use SARR
}

//===----------------------------------------------------------------------===//
// Memory Load/Store Pseudo Instructions
//===----------------------------------------------------------------------===//

// Operand for memory addresses (global, external symbol, frame index)
def memsrc : Operand<i32> {
  let PrintMethod = "printMemOperand";
  let MIOperandInfo = (ops GPR, i32imm);
}

// Load from global/memory address into GPR
let isCodeGenOnly = 1, mayLoad = 1, Defs = [A] in {
  def LOAD32 : Pseudo<(outs GPR:$dst), (ins memsrc:$addr),
                      "# load32 $dst, $addr",
                      [(set GPR:$dst, (load ADDRri:$addr))]>;
                      
  def LOAD32_GLOBAL : Pseudo<(outs GPR:$dst), (ins i32imm:$addr),
                             "# load32 $dst, $addr",
                             [(set GPR:$dst, (load (M65832Wrapper tglobaladdr:$addr)))]>;
}

// Store GPR to global/memory address
let isCodeGenOnly = 1, mayStore = 1, Defs = [A] in {
  def STORE32 : Pseudo<(outs), (ins GPR:$src, memsrc:$addr),
                       "# store32 $src, $addr",
                       [(store GPR:$src, ADDRri:$addr)]>;

  def STORE32_GLOBAL : Pseudo<(outs), (ins GPR:$src, i32imm:$addr),
                              "# store32 $src, $addr",
                              [(store GPR:$src, (M65832Wrapper tglobaladdr:$addr))]>;
}

//===----------------------------------------------------------------------===//
// Load Instructions
// In R mode (register window), DP addresses map to registers R0-R63
//===----------------------------------------------------------------------===//

// LDA - Load Accumulator from Direct Page (register)
def LDA_DP : F1<0xA5, (outs ACC:$dst), (ins DPOp:$src),
               "LDA\t$src",
               []>;

// LDA immediate
def LDA_IMM : F8<0xA9, (outs ACC:$dst), (ins i32imm:$imm),
                "LDA\t#$imm",
                []>;

// LDA absolute (B+$xxxx)
def LDA_ABS : F9<0xAD, (outs ACC:$dst), (ins BankRelOp:$addr),
                "LDA\t$addr",
                []>;

// LDA absolute indexed by X
def LDA_ABS_X : F9<0xBD, (outs ACC:$dst), (ins BankRelOp:$addr),
                  "LDA\t$addr,X",
                  []>;

// LDA indirect (pointer in DP register)
let isCodeGenOnly = 1 in
def LDA_IND : F1<0xB2, (outs ACC:$dst), (ins DPOp:$ptr),
                "LDA\t($ptr)",
                []>;

// LDA indirect indexed by Y
let isCodeGenOnly = 1 in
def LDA_IND_Y : F1<0xB1, (outs ACC:$dst), (ins DPOp:$ptr),
                  "LDA\t($ptr),Y",
                  []>;

// LDX - Load X register from DP
def LDX_DP : F1<0xA6, (outs XREG:$dst), (ins DPOp:$src),
               "LDX\t$src",
               []>;

def LDX_IMM : F8<0xA2, (outs XREG:$dst), (ins i32imm:$imm),
                "LDX\t#$imm",
                []>;

// LDY - Load Y register from DP
def LDY_DP : F1<0xA4, (outs IDXREG:$dst), (ins DPOp:$src),
               "LDY\t$src",
               []>;

def LDY_IMM : F8<0xA0, (outs IDXREG:$dst), (ins i32imm:$imm),
                "LDY\t#$imm",
                []>;

//===----------------------------------------------------------------------===//
// Store Instructions
// In R mode (register window), DP addresses map to registers R0-R63
//===----------------------------------------------------------------------===//

// STA - Store Accumulator to Direct Page (register)
def STA_DP : F1<0x85, (outs), (ins ACC:$src, DPOp:$dst),
               "STA\t$dst",
               []>;

// STA absolute (B+$xxxx)
def STA_ABS : F9<0x8D, (outs), (ins ACC:$src, BankRelOp:$addr),
                "STA\t$addr",
                []>;

// STA absolute indexed by X
def STA_ABS_X : F9<0x9D, (outs), (ins ACC:$src, BankRelOp:$addr),
                  "STA\t$addr,X",
                  []>;

// STA indirect (pointer in DP register)
let isCodeGenOnly = 1 in
def STA_IND : F1<0x92, (outs), (ins ACC:$src, DPOp:$ptr),
                "STA\t($ptr)",
                []>;

// STA indirect indexed by Y
let isCodeGenOnly = 1 in
def STA_IND_Y : F1<0x91, (outs), (ins ACC:$src, DPOp:$ptr),
                  "STA\t($ptr),Y",
                  []>;

// STX - Store X register to DP
def STX_DP : F1<0x86, (outs), (ins XREG:$src, DPOp:$dst),
               "STX\t$dst",
               []>;

// STY - Store Y register to DP
def STY_DP : F1<0x84, (outs), (ins IDXREG:$src, DPOp:$dst),
               "STY\t$dst",
               []>;

// ST zero
def STZ_DP : F1<0x64, (outs), (ins DPOp:$dst),
               "STZ\t$dst",
               []>;

def STZ_ABS : F9<0x9C, (outs), (ins BankRelOp:$addr),
                "STZ\t$addr",
                []>;

//===----------------------------------------------------------------------===//
// Arithmetic Instructions
//===----------------------------------------------------------------------===//

// ADC - Add with Carry (A = A + mem + C)
// For add: CLC; ADC
def ADC_DP : F1<0x65, (outs ACC:$dst), (ins ACC:$src1, DPOp:$src2),
               "ADC\t$src2",
               []> {
  let Constraints = "$src1 = $dst";
  let Defs = [SR];
  let Uses = [SR];
}

def ADC_IMM : F8<0x69, (outs ACC:$dst), (ins ACC:$src1, i32imm:$imm),
                "ADC\t#$imm",
                []> {
  let Constraints = "$src1 = $dst";
  let Defs = [SR];
  let Uses = [SR];
}

// SBC - Subtract with Borrow (A = A - mem - !C)
// For sub: SEC; SBC
def SBC_DP : F1<0xE5, (outs ACC:$dst), (ins ACC:$src1, DPOp:$src2),
               "SBC\t$src2",
               []> {
  let Constraints = "$src1 = $dst";
  let Defs = [SR];
  let Uses = [SR];
}

def SBC_IMM : F8<0xE9, (outs ACC:$dst), (ins ACC:$src1, i32imm:$imm),
                "SBC\t#$imm",
                []> {
  let Constraints = "$src1 = $dst";
  let Defs = [SR];
  let Uses = [SR];
}

// INC/DEC on accumulator
def INC_A : F0<0x1A, (outs ACC:$dst), (ins ACC:$src),
              "INC\tA",
              []> {
  let Constraints = "$src = $dst";
  let Defs = [SR];
}

def DEC_A : F0<0x3A, (outs ACC:$dst), (ins ACC:$src),
              "DEC\tA",
              []> {
  let Constraints = "$src = $dst";
  let Defs = [SR];
}

// INC/DEC on Direct Page (memory/register)
def INC_DP : F1<0xE6, (outs), (ins DPOp:$dst),
               "INC\t$dst",
               []> {
  let Defs = [SR];
}

def DEC_DP : F1<0xC6, (outs), (ins DPOp:$dst),
               "DEC\t$dst",
               []> {
  let Defs = [SR];
}

//===----------------------------------------------------------------------===//
// Logic Instructions
//===----------------------------------------------------------------------===//

// AND
def AND_DP : F1<0x25, (outs ACC:$dst), (ins ACC:$src1, DPOp:$src2),
               "AND\t$src2",
               []> {
  let Constraints = "$src1 = $dst";
  let Defs = [SR];
}

def AND_IMM : F8<0x29, (outs ACC:$dst), (ins ACC:$src1, i32imm:$imm),
                "AND\t#$imm",
                []> {
  let Constraints = "$src1 = $dst";
  let Defs = [SR];
}

// ORA (OR)
def ORA_DP : F1<0x05, (outs ACC:$dst), (ins ACC:$src1, DPOp:$src2),
               "ORA\t$src2",
               []> {
  let Constraints = "$src1 = $dst";
  let Defs = [SR];
}

def ORA_IMM : F8<0x09, (outs ACC:$dst), (ins ACC:$src1, i32imm:$imm),
                "ORA\t#$imm",
                []> {
  let Constraints = "$src1 = $dst";
  let Defs = [SR];
}

// EOR (XOR)
def EOR_DP : F1<0x45, (outs ACC:$dst), (ins ACC:$src1, DPOp:$src2),
               "EOR\t$src2",
               []> {
  let Constraints = "$src1 = $dst";
  let Defs = [SR];
}

def EOR_IMM : F8<0x49, (outs ACC:$dst), (ins ACC:$src1, i32imm:$imm),
                "EOR\t#$imm",
                []> {
  let Constraints = "$src1 = $dst";
  let Defs = [SR];
}

//===----------------------------------------------------------------------===//
// Shift/Rotate Instructions
//===----------------------------------------------------------------------===//

// ASL - Arithmetic Shift Left (A)
def ASL_A : F0<0x0A, (outs ACC:$dst), (ins ACC:$src),
              "ASL\tA",
              []> {
  let Constraints = "$src = $dst";
  let Defs = [SR];
}

// ASL - Arithmetic Shift Left (Direct Page)
def ASL_DP : F1<0x06, (outs), (ins DPOp:$dst),
               "ASL\t$dst",
               []> {
  let Defs = [SR];
}

// LSR - Logical Shift Right (A)
def LSR_A : F0<0x4A, (outs ACC:$dst), (ins ACC:$src),
              "LSR\tA",
              []> {
  let Constraints = "$src = $dst";
  let Defs = [SR];
}

// LSR - Logical Shift Right (Direct Page)
def LSR_DP : F1<0x46, (outs), (ins DPOp:$dst),
               "LSR\t$dst",
               []> {
  let Defs = [SR];
}

// ROL - Rotate Left (Direct Page)
def ROL_DP : F1<0x26, (outs), (ins DPOp:$dst),
               "ROL\t$dst",
               []> {
  let Defs = [SR];
  let Uses = [SR];
}

// ROR - Rotate Right (Direct Page)
def ROR_DP : F1<0x66, (outs), (ins DPOp:$dst),
               "ROR\t$dst",
               []> {
  let Defs = [SR];
  let Uses = [SR];
}

// ROL - Rotate Left through Carry
def ROL_A : F0<0x2A, (outs ACC:$dst), (ins ACC:$src),
              "ROL\tA",
              []> {
  let Constraints = "$src = $dst";
  let Defs = [SR];
  let Uses = [SR];
}

// ROR - Rotate Right through Carry
def ROR_A : F0<0x6A, (outs ACC:$dst), (ins ACC:$src),
              "ROR\tA",
              []> {
  let Constraints = "$src = $dst";
  let Defs = [SR];
  let Uses = [SR];
}

//===----------------------------------------------------------------------===//
// Compare Instructions
//===----------------------------------------------------------------------===//

// CMP - Compare A with memory
def CMP_DP : F1<0xC5, (outs), (ins ACC:$lhs, DPOp:$rhs),
               "CMP\t$rhs",
               []> {
  let Defs = [SR];
}

def CMP_IMM : F8<0xC9, (outs), (ins ACC:$lhs, i32imm:$rhs),
                "CMP\t#$rhs",
                []> {
  let Defs = [SR];
}

//===----------------------------------------------------------------------===//
// Flag Instructions
//===----------------------------------------------------------------------===//

def CLC : F0<0x18, (outs), (ins), "CLC", []> { let Defs = [SR]; }
def SEC : F0<0x38, (outs), (ins), "SEC", []> { let Defs = [SR]; }
def CLI : F0<0x58, (outs), (ins), "CLI", []> { let Defs = [SR]; }
def SEI : F0<0x78, (outs), (ins), "SEI", []> { let Defs = [SR]; }
def CLD : F0<0xD8, (outs), (ins), "CLD", []> { let Defs = [SR]; }
def SED : F0<0xF8, (outs), (ins), "SED", []> { let Defs = [SR]; }
def CLV : F0<0xB8, (outs), (ins), "CLV", []> { let Defs = [SR]; }

//===----------------------------------------------------------------------===//
// Transfer Instructions
//===----------------------------------------------------------------------===//

def TAX : F0<0xAA, (outs XREG:$dst), (ins ACC:$src), "TAX", []> { let Defs = [SR]; }
def TXA : F0<0x8A, (outs ACC:$dst), (ins XREG:$src), "TXA", []> { let Defs = [SR]; }
def TAY : F0<0xA8, (outs), (ins ACC:$src), "TAY", []> { let Defs = [SR, Y]; let Uses = [A]; }
def TYA : F0<0x98, (outs ACC:$dst), (ins), "TYA", []> { let Defs = [SR]; let Uses = [Y]; }
def TSX : F0<0xBA, (outs XREG:$dst), (ins), "TSX", []> { let Defs = [SR]; let Uses = [SP]; }
def TXS : F0<0x9A, (outs), (ins XREG:$src), "TXS", []> { let Defs = [SP]; }

//===----------------------------------------------------------------------===//
// Branch Instructions
//===----------------------------------------------------------------------===//

let isBranch = 1, isTerminator = 1 in {
  def BEQ : F5<0xF0, (outs), (ins brtarget:$target), "BEQ\t$target", []> { let Uses = [SR]; }
  def BNE : F5<0xD0, (outs), (ins brtarget:$target), "BNE\t$target", []> { let Uses = [SR]; }
  def BCS : F5<0xB0, (outs), (ins brtarget:$target), "BCS\t$target", []> { let Uses = [SR]; }
  def BCC : F5<0x90, (outs), (ins brtarget:$target), "BCC\t$target", []> { let Uses = [SR]; }
  def BMI : F5<0x30, (outs), (ins brtarget:$target), "BMI\t$target", []> { let Uses = [SR]; }
  def BPL : F5<0x10, (outs), (ins brtarget:$target), "BPL\t$target", []> { let Uses = [SR]; }
  def BVS : F5<0x70, (outs), (ins brtarget:$target), "BVS\t$target", []> { let Uses = [SR]; }
  def BVC : F5<0x50, (outs), (ins brtarget:$target), "BVC\t$target", []> { let Uses = [SR]; }
  
  let isBarrier = 1 in
  def BRA : F5<0x80, (outs), (ins brtarget:$target), "BRA\t$target",
              [(br bb:$target)]>;
}

// Pseudo for conditional branch - expanded to Bcc in expandPostRAPseudo
// The comparison is done separately via CMPR_DP which sets SR flags
let isCodeGenOnly = 1, isBranch = 1, isTerminator = 1 in {
  def BR_CC_PSEUDO : Pseudo<(outs), (ins i32imm:$cc, brtarget:$target),
                            "# br_cc $cc, $target",
                            [(M65832brcc bb:$target, imm:$cc)]> {
    let Uses = [SR];
  }
}

// Pseudo for conditional select - expanded in expandPostRAPseudo
// dst = (cc) ? trueVal : falseVal
let isCodeGenOnly = 1 in {
  def SELECT_CC_PSEUDO : Pseudo<(outs GPR:$dst), 
                                (ins GPR:$trueVal, GPR:$falseVal, i32imm:$cc),
                                "# select_cc $dst, $trueVal, $falseVal, $cc",
                                [(set GPR:$dst, (M65832selectcc GPR:$trueVal, GPR:$falseVal, imm:$cc))]> {
    let Uses = [SR];
    let Defs = [A];
  }
}

// Long branch
let isBranch = 1, isTerminator = 1, isBarrier = 1 in
def BRL : F6<0x82, (outs), (ins brtarget:$target), "BRL\t$target", []>;

//===----------------------------------------------------------------------===//
// Jump Instructions
//===----------------------------------------------------------------------===//

let isBranch = 1, isTerminator = 1, isBarrier = 1 in {
  def JMP : F3<0x4C, (outs), (ins BankRelOp:$target), "JMP\t$target", []>;
}

let isBranch = 1, isTerminator = 1, isBarrier = 1, isIndirectBranch = 1 in {
  let isCodeGenOnly = 1 in
  def JMP_IND : F3<0x6C, (outs), (ins GPR:$target), "JMP\t$target", []>;
}

//===----------------------------------------------------------------------===//
// Call/Return Instructions
//===----------------------------------------------------------------------===//

let isCall = 1 in {
  def JSR : F3<0x20, (outs), (ins calltarget:$target),
              "JSR\t$target",
              [(M65832call tglobaladdr:$target)]> {
    let Defs = [SP, R0, R1, R2, R3, R4, R5, R6, R7,
                R8, R9, R10, R11, R12, R13, R14, R15, R30,
                R32, R33, R34, R35, R36, R37, R38, R39,
                R40, R41, R42, R43, R44, R45, R46, R47,
                A, X, Y];
    let Uses = [SP];
  }
}

// Pattern for external symbols (library calls like __addsf3, etc.)
def : Pat<(M65832call texternalsym:$target), (JSR texternalsym:$target)>;

let isReturn = 1, isTerminator = 1, isBarrier = 1 in {
  def RTS : F0<0x60, (outs), (ins), "RTS", [(M65832retflag)]> {
    let Uses = [SP];
    let Defs = [SP];
  }
  
  def RTI : F0<0x40, (outs), (ins), "RTI", []> {
    let Uses = [SP];
    let Defs = [SP, SR];
  }
}

//===----------------------------------------------------------------------===//
// Stack Instructions
//===----------------------------------------------------------------------===//

def PHA : F0<0x48, (outs), (ins ACC:$src), "PHA", []> {
  let Defs = [SP];
  let Uses = [SP];
  let mayStore = 1;
}

def PLA : F0<0x68, (outs ACC:$dst), (ins), "PLA", []> {
  let Defs = [SP, SR];
  let Uses = [SP];
  let mayLoad = 1;
}

def PHX : F0<0xDA, (outs), (ins XREG:$src), "PHX", []> {
  let Defs = [SP];
  let Uses = [SP];
  let mayStore = 1;
}

def PLX : F0<0xFA, (outs XREG:$dst), (ins), "PLX", []> {
  let Defs = [SP, SR];
  let Uses = [SP];
  let mayLoad = 1;
}

def PHY : F0<0x5A, (outs), (ins), "PHY", []> {
  let Defs = [SP];
  let Uses = [SP, Y];
  let mayStore = 1;
}

def PLY : F0<0x7A, (outs), (ins), "PLY", []> {
  let Defs = [SP, SR, Y];
  let Uses = [SP];
  let mayLoad = 1;
}

def PHP : F0<0x08, (outs), (ins), "php", []> {
  let Defs = [SP];
  let Uses = [SP, SR];
  let mayStore = 1;
}

def PLP : F0<0x28, (outs), (ins), "plp", []> {
  let Defs = [SP, SR];
  let Uses = [SP];
  let mayLoad = 1;
}

//===----------------------------------------------------------------------===//
// Extended Instructions ($02 prefix)
//===----------------------------------------------------------------------===//

// MUL - Signed Multiply (A = A * [dp])
def MUL_DP : F7_DP<0x00, (outs ACC:$dst, TREG:$hi), (ins ACC:$src1, DPOp:$src2),
                   "mul\t$src2",
                   []> {
  let Constraints = "$src1 = $dst";
  let Defs = [SR, T];
}

// MULU - Unsigned Multiply
def MULU_DP : F7_DP<0x01, (outs ACC:$dst, TREG:$hi), (ins ACC:$src1, DPOp:$src2),
                    "mulu\t$src2",
                    []> {
  let Constraints = "$src1 = $dst";
  let Defs = [SR, T];
}

// DIV - Signed Divide (A = A / [dp], T = remainder)
def DIV_DP : F7_DP<0x04, (outs ACC:$dst, TREG:$rem), (ins ACC:$src1, DPOp:$src2),
                   "div\t$src2",
                   []> {
  let Constraints = "$src1 = $dst";
  let Defs = [SR, T];
}

// DIVU - Unsigned Divide
def DIVU_DP : F7_DP<0x05, (outs ACC:$dst, TREG:$rem), (ins ACC:$src1, DPOp:$src2),
                    "divu\t$src2",
                    []> {
  let Constraints = "$src1 = $dst";
  let Defs = [SR, T];
}

// CAS - Compare and Swap
// if [dp] == X then [dp] = A, Z=1 else X = [dp], Z=0
def CAS_DP : F7_DP<0x10, (outs), (ins ACC:$new, XREG:$expected, DPOp:$addr),
                   "cas\t$addr",
                   []> {
  let Defs = [SR, X];
  let Uses = [A, X];
  let mayLoad = 1;
  let mayStore = 1;
}

// RSET - Enable Register Window (R=1)
def RSET : F7_Imp<0x30, (outs), (ins), "rset", []> {
  let Defs = [SR];
}

// RCLR - Disable Register Window (R=0)
def RCLR : F7_Imp<0x31, (outs), (ins), "rclr", []> {
  let Defs = [SR];
}

// FENCE - Memory Fence
def FENCE : F7_Imp<0x50, (outs), (ins), "fence", []> {
  let hasSideEffects = 1;
}

// TTA - Transfer T to A
def TTA : F7_Imp<0x9A, (outs ACC:$dst), (ins), "tta", []> {
  let Uses = [T];
  let Defs = [SR];
}

// TAT - Transfer A to T
def TAT : F7_Imp<0x9B, (outs), (ins ACC:$src), "tat", []> {
  let Defs = [T];
}

// TRAP - System Call
def TRAP : F7_Imm8<0x40, (outs), (ins imm8:$code), "trap\t#$code", []> {
  let isCall = 1;
  let Defs = [SP, SR];
  let Uses = [SP];
}

//===----------------------------------------------------------------------===//
// NOP
//===----------------------------------------------------------------------===//

def NOP : F0<0xEA, (outs), (ins), "nop", []>;

//===----------------------------------------------------------------------===//
// Pattern Matching
//===----------------------------------------------------------------------===//

// Due to the accumulator-based nature of M65832, most operations need custom 
// lowering in M65832ISelLowering.cpp. Patterns here are minimal.
