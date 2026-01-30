//===-- M65832MCCodeEmitter.cpp - Convert M65832 code to machine code -----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the M65832MCCodeEmitter class.
//
// M65832 instruction encoding:
// - Implied (1 byte): opcode
// - Direct Page (2 bytes): opcode + dp_addr
// - B-relative 16 (3 bytes): opcode + offset_lo + offset_hi (B+offset, B is frame pointer)
// - Relative 16 (3 bytes): opcode + offset_lo + offset_hi (branches in 32-bit mode)
// - Imm32 (5 bytes): opcode + imm[0:31] (32-bit mode)
//
// Extended encodings ($02 prefix):
// - Ext. implied: $02 + ext_op
// - Ext. DP: $02 + ext_op + dp
// - Ext. imm8: $02 + ext_op + imm8
// - Ext. ALU: $02 + ext_op + mode + dest + src...
// - Barrel shifter: $02 $98 op|cnt dest src
// - Extend ops: $02 $99 subop dest src
//
//===----------------------------------------------------------------------===//

#include "M65832FixupKinds.h"
#include "M65832MCTargetDesc.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "mccodeemitter"

static const uint8_t EXT_PREFIX = 0x02;  // Extended opcode prefix

namespace {
class M65832MCCodeEmitter : public MCCodeEmitter {
  const MCInstrInfo &MCII;
  MCContext &Ctx;

  // Get opcode from instruction description
  uint8_t getOpcode(unsigned MIOpcode) const;

public:
  M65832MCCodeEmitter(const MCInstrInfo &MCII, MCContext &Ctx)
      : MCII(MCII), Ctx(Ctx) {}

  ~M65832MCCodeEmitter() override = default;

  void encodeInstruction(const MCInst &MI, SmallVectorImpl<char> &CB,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;

  unsigned getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

  void emitByte(uint8_t Byte, SmallVectorImpl<char> &CB) const {
    CB.push_back(static_cast<char>(Byte));
  }

  void emitLE16(uint16_t Val, SmallVectorImpl<char> &CB) const {
    emitByte(Val & 0xFF, CB);
    emitByte((Val >> 8) & 0xFF, CB);
  }

  void emitLE32(uint32_t Val, SmallVectorImpl<char> &CB) const {
    emitByte(Val & 0xFF, CB);
    emitByte((Val >> 8) & 0xFF, CB);
    emitByte((Val >> 16) & 0xFF, CB);
    emitByte((Val >> 24) & 0xFF, CB);
  }
};
} // end anonymous namespace

MCCodeEmitter *llvm::createM65832MCCodeEmitter(const MCInstrInfo &MCII,
                                                MCContext &Ctx) {
  return new M65832MCCodeEmitter(MCII, Ctx);
}

// Map of instruction opcodes
// This is a simplified lookup - a full implementation would use TableGen
uint8_t M65832MCCodeEmitter::getOpcode(unsigned MIOpcode) const {
  // Standard 6502/65816 opcodes
  switch (MIOpcode) {
  // Load/Store
  case M65832::LDA_DP:    return 0xA5;
  case M65832::LDAr:      return 0xA5;  // GPR variant uses same opcode
  case M65832::LDA_IMM:   return 0xA9;
  case M65832::LDA_ABS:   return 0xAD;
  case M65832::LDA_ABS_X: return 0xBD;
  case M65832::LDA_IND:   return 0xB2;
  case M65832::LDA_IND_r: return 0xB2;  // GPR indirect variant
  case M65832::LDA_IND_Y: return 0xB1;
  case M65832::LDA_IND_Y_r: return 0xB1;  // GPR indirect Y variant
  case M65832::STA_DP:    return 0x85;
  case M65832::STAr:      return 0x85;  // GPR variant uses same opcode
  case M65832::STA_ABS:   return 0x8D;
  case M65832::STA_ABS_X: return 0x9D;
  case M65832::STA_IND:   return 0x92;
  case M65832::STA_IND_r: return 0x92;  // GPR indirect variant
  case M65832::STA_IND_Y: return 0x91;
  case M65832::STA_IND_Y_r: return 0x91;  // GPR indirect Y variant
  case M65832::LDX_DP:    return 0xA6;
  case M65832::LDXr:      return 0xA6;  // GPR variant uses same opcode
  case M65832::LDX_IMM:   return 0xA2;
  case M65832::LDY_DP:    return 0xA4;
  case M65832::LDYr:      return 0xA4;  // GPR variant uses same opcode
  case M65832::LDY_IMM:   return 0xA0;
  case M65832::STX_DP:    return 0x86;
  case M65832::STXr:      return 0x86;  // GPR variant uses same opcode
  case M65832::STY_DP:    return 0x84;
  case M65832::STYr:      return 0x84;  // GPR variant uses same opcode
  case M65832::STZ_DP:    return 0x64;
  case M65832::STZr:      return 0x64;  // GPR variant uses same opcode
  case M65832::STZ_ABS:   return 0x9C;
  
  // Arithmetic
  case M65832::ADC_DP:    return 0x65;
  case M65832::ADCr:      return 0x65;  // GPR variant uses same opcode
  case M65832::ADC_IMM:   return 0x69;
  case M65832::ADC_IND_r: return 0x72;  // GPR indirect variant
  case M65832::ADC_IND_Y_r: return 0x71;  // GPR indirect Y variant
  case M65832::SBC_DP:    return 0xE5;
  case M65832::SBCr:      return 0xE5;  // GPR variant uses same opcode
  case M65832::SBC_IMM:   return 0xE9;
  case M65832::INC_A:     return 0x1A;
  case M65832::DEC_A:     return 0x3A;
  case M65832::INC_DP:    return 0xE6;
  case M65832::INCr:      return 0xE6;  // GPR variant uses same opcode
  case M65832::DEC_DP:    return 0xC6;
  case M65832::DECr:      return 0xC6;  // GPR variant uses same opcode
  
  // Logic
  case M65832::AND_DP:    return 0x25;
  case M65832::ANDr:      return 0x25;  // GPR variant uses same opcode
  case M65832::AND_IMM:   return 0x29;
  case M65832::ORA_DP:    return 0x05;
  case M65832::ORAr:      return 0x05;  // GPR variant uses same opcode
  case M65832::ORA_IMM:   return 0x09;
  case M65832::EOR_DP:    return 0x45;
  case M65832::EORr:      return 0x45;  // GPR variant uses same opcode
  case M65832::EOR_IMM:   return 0x49;
  
  // Shift
  case M65832::ASL_A:     return 0x0A;
  case M65832::ASL_DP:    return 0x06;
  case M65832::ASLr:      return 0x06;  // GPR variant uses same opcode
  case M65832::LSR_A:     return 0x4A;
  case M65832::LSR_DP:    return 0x46;
  case M65832::LSRr:      return 0x46;  // GPR variant uses same opcode
  case M65832::ROL_A:     return 0x2A;
  case M65832::ROL_DP:    return 0x26;
  case M65832::ROLr:      return 0x26;  // GPR variant uses same opcode
  case M65832::ROR_A:     return 0x6A;
  case M65832::ROR_DP:    return 0x66;
  case M65832::RORr:      return 0x66;  // GPR variant uses same opcode
  
  // Compare
  case M65832::CMP_DP:    return 0xC5;
  case M65832::CMPr:      return 0xC5;  // GPR variant uses same opcode
  case M65832::CMP_IMM:   return 0xC9;
  case M65832::SB_IMM:    return 0x22;
  case M65832::SB_DP:     return 0x23;
  
  // Flags
  case M65832::REP:       return 0xC2;
  case M65832::SEP:       return 0xE2;
  case M65832::CLC:       return 0x18;
  case M65832::SEC:       return 0x38;
  case M65832::CLI:       return 0x58;
  case M65832::SEI:       return 0x78;
  case M65832::CLD:       return 0xD8;
  case M65832::SED:       return 0xF8;
  case M65832::CLV:       return 0xB8;
  
  // Transfer
  case M65832::TAX:       return 0xAA;
  case M65832::TXA:       return 0x8A;
  case M65832::TAY:       return 0xA8;
  case M65832::TYA:       return 0x98;
  case M65832::TSX:       return 0xBA;
  case M65832::TXS:       return 0x9A;
  
  // Increment/Decrement X/Y
  case M65832::INX:       return 0xE8;
  case M65832::INY:       return 0xC8;
  case M65832::DEX:       return 0xCA;
  case M65832::DEY:       return 0x88;
  
  // Branch
  case M65832::BEQ:       return 0xF0;
  case M65832::BNE:       return 0xD0;
  case M65832::BCS:       return 0xB0;
  case M65832::BCC:       return 0x90;
  case M65832::BMI:       return 0x30;
  case M65832::BPL:       return 0x10;
  case M65832::BVS:       return 0x70;
  case M65832::BVC:       return 0x50;
  case M65832::BRA:       return 0x80;
  case M65832::BRL:       return 0x82;
  
  // Jump/Call
  case M65832::JMP:       return 0x4C;
  case M65832::JMP_IND:   return 0x6C;
  case M65832::JSR:       return 0x20;
  case M65832::RTS:       return 0x60;
  case M65832::RTI:       return 0x40;
  
  // Stack
  case M65832::PHA:       return 0x48;
  case M65832::PLA:       return 0x68;
  case M65832::PHX:       return 0xDA;
  case M65832::PLX:       return 0xFA;
  case M65832::PHY:       return 0x5A;
  case M65832::PLY:       return 0x7A;
  case M65832::PHP:       return 0x08;
  case M65832::PLP:       return 0x28;
  case M65832::PHB:       return 0x8B;
  case M65832::PLB:       return 0xAB;
  
  // Misc
  case M65832::NOP:       return 0xEA;
  case M65832::STP:       return 0xDB;
  case M65832::WAI:       return 0xCB;
  
  // Extended instructions ($02 prefix)
  case M65832::MUL_DP:    return 0x00;
  case M65832::MULU_DP:   return 0x01;
  case M65832::DIV_DP:    return 0x04;
  case M65832::DIVU_DP:   return 0x05;
  case M65832::CAS_DP:    return 0x10;
  case M65832::RSET:      return 0x30;
  case M65832::RCLR:      return 0x31;
  case M65832::TRAP:      return 0x40;
  case M65832::FENCE:     return 0x50;
  case M65832::FENCER:    return 0x51;
  case M65832::FENCEW:    return 0x52;
  case M65832::TAB:       return 0x91;
  case M65832::TBA:       return 0x92;
  case M65832::TXB:       return 0x93;
  case M65832::TBX:       return 0x94;
  case M65832::TYB:       return 0x95;
  case M65832::TBY:       return 0x96;
  case M65832::TSPB:      return 0xA4;
  case M65832::TTA:       return 0x9A;
  case M65832::TAT:       return 0x9B;
  
  // Extended ALU opcodes ($02 $80-$97)
  case M65832::MOVR_DP:   return 0x80;
  case M65832::LDR_IMM:   return 0x80;
  case M65832::ADDR_DP:   return 0x82;
  case M65832::ADDR_IMM:  return 0x82;
  case M65832::SUBR_DP:   return 0x83;
  case M65832::SUBR_IMM:  return 0x83;
  case M65832::ANDR_DP:   return 0x84;
  case M65832::ANDR_IMM:  return 0x84;
  case M65832::ORAR_DP:   return 0x85;
  case M65832::ORAR_IMM:  return 0x85;
  case M65832::EORR_DP:   return 0x86;
  case M65832::EORR_IMM:  return 0x86;
  case M65832::CMPR_DP:   return 0x87;
  case M65832::CMPR_IMM:  return 0x87;
  
  // Extended ALU - byte operations (LD.B/ST.B)
  case M65832::LDB_DP:    return 0x80;
  case M65832::LDB_ABS:   return 0x80;
  case M65832::LDB_IND_Y: return 0x80;
  case M65832::STB_DP:    return 0x81;
  case M65832::STB_ABS:   return 0x81;
  case M65832::STB_IND_Y: return 0x81;
  
  // Extended ALU - word operations (LD.W/ST.W)
  case M65832::LDW_DP:    return 0x80;
  case M65832::LDW_ABS:   return 0x80;
  case M65832::LDW_IND_Y: return 0x80;
  case M65832::STW_DP:    return 0x81;
  case M65832::STW_ABS:   return 0x81;
  case M65832::STW_IND_Y: return 0x81;
  
  // Barrel shifter - opcode encodes op|cnt
  case M65832::SHLR:      return 0x00;
  case M65832::SHRR:      return 0x20;
  case M65832::SARR:      return 0x40;
  case M65832::ROLR:      return 0x60;
  case M65832::RORR:      return 0x80;
  case M65832::SHLR_VAR:  return 0x1F;
  case M65832::SHRR_VAR:  return 0x3F;
  case M65832::SARR_VAR:  return 0x5F;
  
  // Extend operations
  case M65832::SEXT8:     return 0x00;
  case M65832::SEXT16:    return 0x01;
  case M65832::ZEXT8:     return 0x02;
  case M65832::ZEXT16:    return 0x03;
  case M65832::CLZ:       return 0x04;
  case M65832::CTZ:       return 0x05;
  case M65832::POPCNT:    return 0x06;
  
  default:
    return 0xEA; // NOP as fallback
  }
}

void M65832MCCodeEmitter::encodeInstruction(const MCInst &MI,
                                              SmallVectorImpl<char> &CB,
                                              SmallVectorImpl<MCFixup> &Fixups,
                                              const MCSubtargetInfo &STI) const {
  const MCInstrDesc &Desc = MCII.get(MI.getOpcode());
  unsigned Size = Desc.getSize();
  unsigned MIOp = MI.getOpcode();
  uint8_t Opcode = getOpcode(MI.getOpcode());

  // Helper to evaluate constant expressions from inline assembly
  auto tryEvaluateConstant = [&](const MCExpr *Expr, int64_t &Value) -> bool {
    if (const auto *CE = dyn_cast<MCConstantExpr>(Expr)) {
      Value = CE->getValue();
      return true;
    }
    return Expr->evaluateAsAbsolute(Value);
  };

  auto emitImm8 = [&](const MCOperand &MO, unsigned Offset) {
    if (MO.isImm()) {
      emitByte(static_cast<uint8_t>(MO.getImm()), CB);
    } else if (MO.isExpr()) {
      int64_t Value;
      if (tryEvaluateConstant(MO.getExpr(), Value)) {
        emitByte(static_cast<uint8_t>(Value), CB);
      } else {
        Fixups.push_back(
            MCFixup::create(Offset, MO.getExpr(), MCFixupKind(FK_Data_1)));
        emitByte(0, CB);
      }
    } else {
      emitByte(0, CB);
    }
  };

  auto emitImm16 = [&](const MCOperand &MO, unsigned Offset, bool IsPCRel = false) {
    if (MO.isImm()) {
      int64_t Imm = MO.getImm();
      // For PC-relative branches, the immediate from BuildMI is "*+N" style
      // (N bytes from instruction start). Convert to CPU offset format:
      // CPU computes target = PC + offset, where PC = instruction_addr + 3
      // So if we want target = instruction_addr + N, offset = N - 3
      if (IsPCRel) {
        Imm -= 3;
      }
      emitLE16(static_cast<uint16_t>(Imm), CB);
    } else if (MO.isExpr()) {
      // Try to evaluate constant expressions (from inline assembly)
      int64_t Value;
      if (!IsPCRel && tryEvaluateConstant(MO.getExpr(), Value)) {
        emitLE16(static_cast<uint16_t>(Value), CB);
      } else {
        // Use target-specific PC-relative fixup kind for branches
        // Note: Setting PCRel=true causes crashes in MCAssembler layout,
        // so we use a custom fixup kind to identify PC-relative fixups instead.
        MCFixupKind Kind = IsPCRel ? MCFixupKind(M65832::fixup_m65832_pcrel_16) 
                                   : MCFixupKind(FK_Data_2);
        const MCExpr *Expr = MO.getExpr();
        
        // For PC-relative branches, the M65832 calculates target from the 
        // instruction END (opcode + 3), but the relocation is at opcode + 1.
        // LLD computes: val = target - reloc_addr
        // We need: offset = target - (reloc_addr + 2)
        // So add -2 to the expression.
        if (IsPCRel) {
          Expr = MCBinaryExpr::createAdd(
              Expr, MCConstantExpr::create(-2, Ctx), Ctx);
        }
        
        Fixups.push_back(MCFixup::create(Offset, Expr, Kind, /*PCRel=*/false));
        emitLE16(0, CB);
      }
    } else {
      emitLE16(0, CB);
    }
  };

  auto emitImm32 = [&](const MCOperand &MO, unsigned Offset) {
    if (MO.isImm()) {
      emitLE32(static_cast<uint32_t>(MO.getImm()), CB);
    } else if (MO.isExpr()) {
      // Try to evaluate constant expressions (from inline assembly)
      int64_t Value;
      if (tryEvaluateConstant(MO.getExpr(), Value)) {
        emitLE32(static_cast<uint32_t>(Value), CB);
      } else {
        Fixups.push_back(
            MCFixup::create(Offset, MO.getExpr(), MCFixupKind(FK_Data_4)));
        emitLE32(0, CB);
      }
    } else {
      emitLE32(0, CB);
    }
  };

  auto regToDP = [&](unsigned Reg) -> uint8_t {
    if (Reg >= M65832::R0 && Reg <= M65832::R63)
      return static_cast<uint8_t>((Reg - M65832::R0) * 4);
    return 0;
  };

  auto emitDPOp = [&](const MCOperand &MO, unsigned Offset) {
    if (MO.isReg()) {
      emitByte(regToDP(MO.getReg()), CB);
    } else {
      emitImm8(MO, Offset);
    }
  };

  // Extended instructions ($02 prefix)
  // Note: PHB and PLB are NOT extended - they use standard 65816 opcodes 0x8B and 0xAB
  switch (MIOp) {
  case M65832::MUL_DP:
  case M65832::MULU_DP:
  case M65832::DIV_DP:
  case M65832::DIVU_DP:
  case M65832::CAS_DP: {
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    const MCOperand &MO = MI.getOperand(MI.getNumOperands() - 1);
    emitImm8(MO, 2);
    return;
  }
  case M65832::RSET:
  case M65832::RCLR:
  case M65832::FENCE:
  case M65832::FENCER:
  case M65832::FENCEW:
  case M65832::TAB:
  case M65832::TBA:
  case M65832::TXB:
  case M65832::TBX:
  case M65832::TYB:
  case M65832::TBY:
  case M65832::TSPB:
  case M65832::TTA:
  case M65832::TAT: {
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    return;
  }
  case M65832::SB_DP: {
    // SB dp - Set B from direct page (3 bytes: $02 $23 dp)
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    const MCOperand &MO = MI.getOperand(0);
    emitDPOp(MO, 2);
    return;
  }
  case M65832::SB_IMM: {
    // SB #imm32 - Set B from immediate (6 bytes: $02 $22 imm32)
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    const MCOperand &MO = MI.getOperand(0);
    emitImm32(MO, 2);
    return;
  }
  case M65832::TRAP: {
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    const MCOperand &MO = MI.getOperand(MI.getNumOperands() - 1);
    emitImm8(MO, 2);
    return;
  }

  // Extended ALU (dp source)
  case M65832::MOVR_DP:
  case M65832::ADDR_DP:
  case M65832::SUBR_DP:
  case M65832::ANDR_DP:
  case M65832::ORAR_DP:
  case M65832::EORR_DP:
  case M65832::CMPR_DP: {
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    emitByte(0xA0, CB); // size=long, target=Rn, addr_mode=dp
    if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg())
      emitByte(regToDP(MI.getOperand(0).getReg()), CB);
    else
      emitByte(0, CB);
    const MCOperand &SrcOp = MI.getOperand(MI.getNumOperands() - 1);
    emitDPOp(SrcOp, 4);
    return;
  }

  // Extended ALU (immediate)
  case M65832::LDR_IMM:
  case M65832::ADDR_IMM:
  case M65832::SUBR_IMM:
  case M65832::ANDR_IMM:
  case M65832::ORAR_IMM:
  case M65832::EORR_IMM:
  case M65832::CMPR_IMM: {
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    emitByte(0xB8, CB); // size=long, target=Rn, addr_mode=imm
    if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg())
      emitByte(regToDP(MI.getOperand(0).getReg()), CB);
    else
      emitByte(0, CB);
    const MCOperand &ImmOp = MI.getOperand(MI.getNumOperands() - 1);
    emitImm32(ImmOp, 4);
    return;
  }

  // Extended ALU - BYTE (8-bit) operations
  // Mode byte: [size:2=00][target:1=1][addr_mode:5]
  // addr_mode: 0=dp, 4=(dp)Y, 8=abs
  case M65832::LDB_DP: {
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    emitByte(0x20, CB); // size=byte(00), target=Rn(1), addr_mode=dp(0)
    if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg())
      emitByte(regToDP(MI.getOperand(0).getReg()), CB);
    else
      emitByte(0, CB);
    const MCOperand &SrcOp = MI.getOperand(MI.getNumOperands() - 1);
    emitDPOp(SrcOp, 4);
    return;
  }
  case M65832::STB_DP: {
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    emitByte(0x20, CB); // size=byte(00), target=Rn(1), addr_mode=dp(0)
    // For ST: dest_dp = value register, src_operand = dest address register
    // Operand 0 = src (value to store), Operand 1 = dst (address)
    if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg())
      emitByte(regToDP(MI.getOperand(0).getReg()), CB);  // value reg (dest_dp)
    else
      emitByte(0, CB);
    if (MI.getNumOperands() >= 2 && MI.getOperand(1).isReg())
      emitByte(regToDP(MI.getOperand(1).getReg()), CB);  // dest address
    else
      emitByte(0, CB);
    return;
  }
  case M65832::LDB_ABS: {
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    emitByte(0x28, CB); // size=byte(00), target=Rn(1), addr_mode=abs(8)
    if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg())
      emitByte(regToDP(MI.getOperand(0).getReg()), CB);
    else
      emitByte(0, CB);
    const MCOperand &AddrOp = MI.getOperand(MI.getNumOperands() - 1);
    emitImm16(AddrOp, 4);
    return;
  }
  case M65832::STB_ABS: {
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    emitByte(0x28, CB); // size=byte(00), target=Rn(1), addr_mode=abs(8)
    // For stores: emit dest address register, then src register address
    if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg())
      emitByte(regToDP(MI.getOperand(0).getReg()), CB);
    else
      emitByte(0, CB);
    const MCOperand &AddrOp = MI.getOperand(MI.getNumOperands() - 1);
    emitImm16(AddrOp, 4);
    return;
  }
  case M65832::LDB_IND_Y: {
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    emitByte(0x24, CB); // size=byte(00), target=Rn(1), addr_mode=(dp)Y(4)
    if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg())
      emitByte(regToDP(MI.getOperand(0).getReg()), CB);
    else
      emitByte(0, CB);
    if (MI.getNumOperands() >= 2 && MI.getOperand(1).isReg())
      emitByte(regToDP(MI.getOperand(1).getReg()), CB);
    else
      emitByte(0, CB);
    return;
  }
  case M65832::STB_IND_Y: {
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    emitByte(0x24, CB); // size=byte(00), target=Rn(1), addr_mode=(dp)Y(4)
    // For ST: dest_dp = value register, src_operand = address register
    // Operand 0 = src (value to store), Operand 1 = base (address)
    if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg())
      emitByte(regToDP(MI.getOperand(0).getReg()), CB);  // value reg (dest_dp)
    else
      emitByte(0, CB);
    if (MI.getNumOperands() >= 2 && MI.getOperand(1).isReg())
      emitByte(regToDP(MI.getOperand(1).getReg()), CB);  // address reg (src)
    else
      emitByte(0, CB);
    return;
  }

  // Extended ALU - WORD (16-bit) operations
  // Mode byte: [size:2=01][target:1=1][addr_mode:5]
  case M65832::LDW_DP: {
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    emitByte(0x60, CB); // size=word(01), target=Rn(1), addr_mode=dp(0)
    if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg())
      emitByte(regToDP(MI.getOperand(0).getReg()), CB);
    else
      emitByte(0, CB);
    const MCOperand &SrcOp = MI.getOperand(MI.getNumOperands() - 1);
    emitDPOp(SrcOp, 4);
    return;
  }
  case M65832::STW_DP: {
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    emitByte(0x60, CB); // size=word(01), target=Rn(1), addr_mode=dp(0)
    // For ST: dest_dp = value register, src_operand = dest address register
    // Operand 0 = src (value to store), Operand 1 = dst (address)
    if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg())
      emitByte(regToDP(MI.getOperand(0).getReg()), CB);  // value reg (dest_dp)
    else
      emitByte(0, CB);
    if (MI.getNumOperands() >= 2 && MI.getOperand(1).isReg())
      emitByte(regToDP(MI.getOperand(1).getReg()), CB);  // dest address
    else
      emitByte(0, CB);
    return;
  }
  case M65832::LDW_ABS: {
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    emitByte(0x68, CB); // size=word(01), target=Rn(1), addr_mode=abs(8)
    if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg())
      emitByte(regToDP(MI.getOperand(0).getReg()), CB);
    else
      emitByte(0, CB);
    const MCOperand &AddrOp = MI.getOperand(MI.getNumOperands() - 1);
    emitImm16(AddrOp, 4);
    return;
  }
  case M65832::STW_ABS: {
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    emitByte(0x68, CB); // size=word(01), target=Rn(1), addr_mode=abs(8)
    // For stores: emit src register, then address
    if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg())
      emitByte(regToDP(MI.getOperand(0).getReg()), CB);
    else
      emitByte(0, CB);
    const MCOperand &AddrOp = MI.getOperand(MI.getNumOperands() - 1);
    emitImm16(AddrOp, 4);
    return;
  }
  case M65832::LDW_IND_Y: {
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    emitByte(0x64, CB); // size=word(01), target=Rn(1), addr_mode=(dp)Y(4)
    if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg())
      emitByte(regToDP(MI.getOperand(0).getReg()), CB);
    else
      emitByte(0, CB);
    if (MI.getNumOperands() >= 2 && MI.getOperand(1).isReg())
      emitByte(regToDP(MI.getOperand(1).getReg()), CB);
    else
      emitByte(0, CB);
    return;
  }
  case M65832::STW_IND_Y: {
    emitByte(EXT_PREFIX, CB);
    emitByte(Opcode, CB);
    emitByte(0x64, CB); // size=word(01), target=Rn(1), addr_mode=(dp)Y(4)
    // For ST: dest_dp = value register, src_operand = address register
    // Operand 0 = src (value to store), Operand 1 = base (address)
    if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg())
      emitByte(regToDP(MI.getOperand(0).getReg()), CB);  // value reg (dest_dp)
    else
      emitByte(0, CB);
    if (MI.getNumOperands() >= 2 && MI.getOperand(1).isReg())
      emitByte(regToDP(MI.getOperand(1).getReg()), CB);  // address reg (src)
    else
      emitByte(0, CB);
    return;
  }

  // Barrel shifter ($02 $98 op|cnt dest src)
  case M65832::SHLR:
  case M65832::SHRR:
  case M65832::SARR:
  case M65832::ROLR:
  case M65832::RORR:
  case M65832::SHLR_VAR:
  case M65832::SHRR_VAR:
  case M65832::SARR_VAR: {
    emitByte(EXT_PREFIX, CB);
    emitByte(0x98, CB);
    uint8_t opcnt = Opcode;
    if (MI.getNumOperands() >= 3 && MI.getOperand(2).isImm())
      opcnt = (Opcode & 0xE0) | (MI.getOperand(2).getImm() & 0x1F);
    emitByte(opcnt, CB);
    if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg())
      emitByte(regToDP(MI.getOperand(0).getReg()), CB);
    else
      emitByte(0, CB);
    if (MI.getNumOperands() >= 2 && MI.getOperand(1).isReg())
      emitByte(regToDP(MI.getOperand(1).getReg()), CB);
    else
      emitByte(0, CB);
    return;
  }

  // Extend ops ($02 $99 subop dest src)
  case M65832::SEXT8:
  case M65832::SEXT16:
  case M65832::ZEXT8:
  case M65832::ZEXT16:
  case M65832::CLZ:
  case M65832::CTZ:
  case M65832::POPCNT: {
    emitByte(EXT_PREFIX, CB);
    emitByte(0x99, CB);
    emitByte(Opcode, CB);
    if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg())
      emitByte(regToDP(MI.getOperand(0).getReg()), CB);
    else
      emitByte(0, CB);
    if (MI.getNumOperands() >= 2 && MI.getOperand(1).isReg())
      emitByte(regToDP(MI.getOperand(1).getReg()), CB);
    else
      emitByte(0, CB);
    return;
  }

  // FPU register-indirect load/store ($02 $B4/$B5 $nm)
  // LDF Fn, (Rm) / STF Fn, (Rm)
  // Encoding: $02 $B4 $nm for load, $02 $B5 $nm for store
  // n = FPU reg (0-15), m = GPR reg number / 4
  case M65832::LDF_ind: {
    emitByte(EXT_PREFIX, CB);
    emitByte(0xB4, CB);  // LDF indirect opcode
    // Get FPU register (F0-F15) -> n
    unsigned FReg = MI.getOperand(0).getReg();
    unsigned n = FReg - M65832::F0;
    // Get GPR register (R0-R63) -> m = DP offset / 4
    unsigned GReg = MI.getOperand(1).getReg();
    unsigned m = regToDP(GReg) / 4;
    emitByte((n << 4) | m, CB);
    return;
  }
  case M65832::STF_ind: {
    emitByte(EXT_PREFIX, CB);
    emitByte(0xB5, CB);  // STF indirect opcode
    // Get FPU register (F0-F15) -> n
    unsigned FReg = MI.getOperand(0).getReg();
    unsigned n = FReg - M65832::F0;
    // Get GPR register (R0-R63) -> m = DP offset / 4
    unsigned GReg = MI.getOperand(1).getReg();
    unsigned m = regToDP(GReg) / 4;
    emitByte((n << 4) | m, CB);
    return;
  }
  case M65832::LDF_S_ind: {
    emitByte(EXT_PREFIX, CB);
    emitByte(0xBA, CB);  // LDF.S indirect opcode
    unsigned FReg = MI.getOperand(0).getReg();
    unsigned n = FReg - M65832::F0;
    unsigned GReg = MI.getOperand(1).getReg();
    unsigned m = regToDP(GReg) / 4;
    emitByte((n << 4) | m, CB);
    return;
  }
  case M65832::STF_S_ind: {
    emitByte(EXT_PREFIX, CB);
    emitByte(0xBB, CB);  // STF.S indirect opcode
    unsigned FReg = MI.getOperand(0).getReg();
    unsigned n = FReg - M65832::F0;
    unsigned GReg = MI.getOperand(1).getReg();
    unsigned m = regToDP(GReg) / 4;
    emitByte((n << 4) | m, CB);
    return;
  }

  // FPU binary arithmetic ($02 opcode $nm) - single precision
  case M65832::FADD_S:
  case M65832::FSUB_S:
  case M65832::FMUL_S:
  case M65832::FDIV_S: {
    emitByte(EXT_PREFIX, CB);
    unsigned opcodeVal = 0xC0;
    switch (MIOp) {
    case M65832::FADD_S: opcodeVal = 0xC0; break;
    case M65832::FSUB_S: opcodeVal = 0xC1; break;
    case M65832::FMUL_S: opcodeVal = 0xC2; break;
    case M65832::FDIV_S: opcodeVal = 0xC3; break;
    }
    emitByte(opcodeVal, CB);
    unsigned DstFReg = MI.getOperand(0).getReg();
    unsigned SrcFReg = MI.getOperand(2).getReg();  // Operand 1 is $Fd tied to $dst
    unsigned d = DstFReg - M65832::F0;
    unsigned s = SrcFReg - M65832::F0;
    emitByte((d << 4) | s, CB);
    return;
  }

  // FPU binary arithmetic ($02 opcode $nm) - double precision
  case M65832::FADD_D:
  case M65832::FSUB_D:
  case M65832::FMUL_D:
  case M65832::FDIV_D: {
    emitByte(EXT_PREFIX, CB);
    unsigned opcodeVal = 0xD0;
    switch (MIOp) {
    case M65832::FADD_D: opcodeVal = 0xD0; break;
    case M65832::FSUB_D: opcodeVal = 0xD1; break;
    case M65832::FMUL_D: opcodeVal = 0xD2; break;
    case M65832::FDIV_D: opcodeVal = 0xD3; break;
    }
    emitByte(opcodeVal, CB);
    unsigned DstFReg = MI.getOperand(0).getReg();
    unsigned SrcFReg = MI.getOperand(2).getReg();  // Operand 1 is $Fd tied to $dst
    unsigned d = DstFReg - M65832::F0;
    unsigned s = SrcFReg - M65832::F0;
    emitByte((d << 4) | s, CB);
    return;
  }

  // FPU unary ops ($02 opcode $nm) - single precision
  case M65832::FNEG_S:
  case M65832::FABS_S:
  case M65832::FSQRT_S: {
    emitByte(EXT_PREFIX, CB);
    unsigned opcodeVal = 0xC4;
    switch (MIOp) {
    case M65832::FNEG_S: opcodeVal = 0xC4; break;
    case M65832::FABS_S: opcodeVal = 0xC5; break;
    case M65832::FSQRT_S: opcodeVal = 0xCA; break;
    }
    emitByte(opcodeVal, CB);
    unsigned DstFReg = MI.getOperand(0).getReg();
    unsigned SrcFReg = MI.getOperand(1).getReg();
    unsigned d = DstFReg - M65832::F0;
    unsigned s = SrcFReg - M65832::F0;
    emitByte((d << 4) | s, CB);
    return;
  }

  // FPU unary ops ($02 opcode $nm) - double precision
  case M65832::FNEG_D:
  case M65832::FABS_D:
  case M65832::FSQRT_D: {
    emitByte(EXT_PREFIX, CB);
    unsigned opcodeVal = 0xD4;
    switch (MIOp) {
    case M65832::FNEG_D: opcodeVal = 0xD4; break;
    case M65832::FABS_D: opcodeVal = 0xD5; break;
    case M65832::FSQRT_D: opcodeVal = 0xDA; break;
    }
    emitByte(opcodeVal, CB);
    unsigned DstFReg = MI.getOperand(0).getReg();
    unsigned SrcFReg = MI.getOperand(1).getReg();
    unsigned d = DstFReg - M65832::F0;
    unsigned s = SrcFReg - M65832::F0;
    emitByte((d << 4) | s, CB);
    return;
  }

  // FPU compare ($02 opcode $nm)
  case M65832::FCMP_S: {
    emitByte(EXT_PREFIX, CB);
    emitByte(0xC6, CB);
    unsigned FdReg = MI.getOperand(0).getReg();
    unsigned FsReg = MI.getOperand(1).getReg();
    unsigned d = FdReg - M65832::F0;
    unsigned s = FsReg - M65832::F0;
    emitByte((d << 4) | s, CB);
    return;
  }
  case M65832::FCMP_D: {
    emitByte(EXT_PREFIX, CB);
    emitByte(0xD6, CB);
    unsigned FdReg = MI.getOperand(0).getReg();
    unsigned FsReg = MI.getOperand(1).getReg();
    unsigned d = FdReg - M65832::F0;
    unsigned s = FsReg - M65832::F0;
    emitByte((d << 4) | s, CB);
    return;
  }

  // FPU move ($02 opcode $nm)
  case M65832::FMOV_S: {
    emitByte(EXT_PREFIX, CB);
    emitByte(0xC9, CB);
    unsigned DstFReg = MI.getOperand(0).getReg();
    unsigned SrcFReg = MI.getOperand(1).getReg();
    unsigned d = DstFReg - M65832::F0;
    unsigned s = SrcFReg - M65832::F0;
    emitByte((d << 4) | s, CB);
    return;
  }
  case M65832::FMOV_D: {
    emitByte(EXT_PREFIX, CB);
    emitByte(0xD9, CB);
    unsigned DstFReg = MI.getOperand(0).getReg();
    unsigned SrcFReg = MI.getOperand(1).getReg();
    unsigned d = DstFReg - M65832::F0;
    unsigned s = SrcFReg - M65832::F0;
    emitByte((d << 4) | s, CB);
    return;
  }

  // FPU real F2I/I2F instructions
  case M65832::F2I_S_real: {
    emitByte(EXT_PREFIX, CB);
    emitByte(0xC7, CB);  // F2I.S opcode
    unsigned FReg = MI.getOperand(0).getReg();
    unsigned f = FReg - M65832::F0;
    emitByte((f << 4) | f, CB);  // reg-byte (same src/dst for F2I)
    return;
  }
  case M65832::F2I_D_real: {
    emitByte(EXT_PREFIX, CB);
    emitByte(0xD7, CB);  // F2I.D opcode
    unsigned FReg = MI.getOperand(0).getReg();
    unsigned f = FReg - M65832::F0;
    emitByte((f << 4) | f, CB);
    return;
  }
  case M65832::I2F_S_real: {
    emitByte(EXT_PREFIX, CB);
    emitByte(0xC8, CB);  // I2F.S opcode
    unsigned FReg = MI.getOperand(0).getReg();
    unsigned f = FReg - M65832::F0;
    emitByte((f << 4) | f, CB);
    return;
  }
  case M65832::I2F_D_real: {
    emitByte(EXT_PREFIX, CB);
    emitByte(0xD8, CB);  // I2F.D opcode
    unsigned FReg = MI.getOperand(0).getReg();
    unsigned f = FReg - M65832::F0;
    emitByte((f << 4) | f, CB);
    return;
  }

  default:
    break;
  }

  // Standard encodings by size
  switch (Size) {
  case 0:
  case 1:
    emitByte(Opcode, CB);
    break;
  case 2: {
    emitByte(Opcode, CB);
    if (MI.getNumOperands() > 0) {
      const MCOperand &MO = MI.getOperand(MI.getNumOperands() - 1);
      // Use emitDPOp to handle both register operands (R0-R63) and immediates
      emitDPOp(MO, 1);
    } else {
      emitByte(0, CB);
    }
    break;
  }
  case 3: {
    emitByte(Opcode, CB);
    if (MI.getNumOperands() > 0) {
      const MCOperand &MO = MI.getOperand(MI.getNumOperands() - 1);
      // Check if this is a branch instruction (needs PC-relative fixup)
      bool IsBranch = false;
      switch (MIOp) {
      case M65832::BEQ:
      case M65832::BNE:
      case M65832::BCS:
      case M65832::BCC:
      case M65832::BMI:
      case M65832::BPL:
      case M65832::BVS:
      case M65832::BVC:
      case M65832::BRA:
      case M65832::BRL:
        IsBranch = true;
        break;
      default:
        break;
      }
      emitImm16(MO, 1, IsBranch);
    } else {
      emitLE16(0, CB);
    }
    break;
  }
  case 5: {
    emitByte(Opcode, CB);
    if (MI.getNumOperands() > 0) {
      const MCOperand &MO = MI.getOperand(MI.getNumOperands() - 1);
      emitImm32(MO, 1);
    } else {
      emitLE32(0, CB);
    }
    break;
  }
  default:
    for (unsigned i = 0; i < Size; ++i)
      emitByte(0xEA, CB);
    break;
  }
}

unsigned M65832MCCodeEmitter::getMachineOpValue(const MCInst &MI,
                                                  const MCOperand &MO,
                                                  SmallVectorImpl<MCFixup> &Fixups,
                                                  const MCSubtargetInfo &STI) const {
  if (MO.isReg())
    return Ctx.getRegisterInfo()->getEncodingValue(MO.getReg());
  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm());
  
  // Handle expressions
  assert(MO.isExpr() && "Unknown operand type");
  
  // Add a fixup for the expression
  const MCExpr *Expr = MO.getExpr();
  Fixups.push_back(MCFixup::create(0, Expr, MCFixupKind(FK_Data_4)));
  
  return 0;
}
