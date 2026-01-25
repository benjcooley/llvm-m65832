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
// - Absolute 16 (3 bytes): opcode + addr_lo + addr_hi
// - Relative 8 (2 bytes): opcode + offset
// - WID + Imm32 (5 bytes): 0x42 + opcode + imm[0:31]
// - WID + Abs32 (6 bytes): 0x42 + opcode + addr[0:31]
//
//===----------------------------------------------------------------------===//

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

// WID prefix for 32-bit operations
static const uint8_t WID_PREFIX = 0x42;

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
  case M65832::LDA_IMM:   return 0xA9;
  case M65832::LDA_ABS:   return 0xAD;
  case M65832::LDA_ABS_X: return 0xBD;
  case M65832::LDA_IND:   return 0xB2;
  case M65832::LDA_IND_Y: return 0xB1;
  case M65832::STA_DP:    return 0x85;
  case M65832::STA_ABS:   return 0x8D;
  case M65832::STA_ABS_X: return 0x9D;
  case M65832::STA_IND:   return 0x92;
  case M65832::STA_IND_Y: return 0x91;
  case M65832::LDX_DP:    return 0xA6;
  case M65832::LDX_IMM:   return 0xA2;
  case M65832::LDY_DP:    return 0xA4;
  case M65832::LDY_IMM:   return 0xA0;
  case M65832::STX_DP:    return 0x86;
  case M65832::STY_DP:    return 0x84;
  case M65832::STZ_DP:    return 0x64;
  case M65832::STZ_ABS:   return 0x9C;
  
  // Arithmetic
  case M65832::ADC_DP:    return 0x65;
  case M65832::ADC_IMM:   return 0x69;
  case M65832::SBC_DP:    return 0xE5;
  case M65832::SBC_IMM:   return 0xE9;
  case M65832::INC_A:     return 0x1A;
  case M65832::DEC_A:     return 0x3A;
  case M65832::INC_DP:    return 0xE6;
  case M65832::DEC_DP:    return 0xC6;
  
  // Logic
  case M65832::AND_DP:    return 0x25;
  case M65832::AND_IMM:   return 0x29;
  case M65832::ORA_DP:    return 0x05;
  case M65832::ORA_IMM:   return 0x09;
  case M65832::EOR_DP:    return 0x45;
  case M65832::EOR_IMM:   return 0x49;
  
  // Shift
  case M65832::ASL_A:     return 0x0A;
  case M65832::ASL_DP:    return 0x06;
  case M65832::LSR_A:     return 0x4A;
  case M65832::LSR_DP:    return 0x46;
  case M65832::ROL_A:     return 0x2A;
  case M65832::ROL_DP:    return 0x26;
  case M65832::ROR_A:     return 0x6A;
  case M65832::ROR_DP:    return 0x66;
  
  // Compare
  case M65832::CMP_DP:    return 0xC5;
  case M65832::CMP_IMM:   return 0xC9;
  
  // Flags
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
  
  // Misc
  case M65832::NOP:       return 0xEA;
  
  // Extended instructions - these have special encoding
  // Return 0 as the opcode is embedded in the instruction format
  case M65832::ADDR_DP:   return 0x11;
  case M65832::SUBR_DP:   return 0x21;
  case M65832::ANDR_DP:   return 0x31;
  case M65832::ORAR_DP:   return 0x41;
  case M65832::EORR_DP:   return 0x51;
  case M65832::CMPR_DP:   return 0x61;
  case M65832::MOVR_DP:   return 0x01;
  case M65832::ADDR_IMM:  return 0x12;
  case M65832::SUBR_IMM:  return 0x22;
  case M65832::ANDR_IMM:  return 0x32;
  case M65832::ORAR_IMM:  return 0x42;
  case M65832::EORR_IMM:  return 0x52;
  case M65832::CMPR_IMM:  return 0x62;
  case M65832::LDR_IMM:   return 0x02;
  
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
  uint8_t Opcode = getOpcode(MI.getOpcode());
  
  // Get the instruction size category based on Size
  switch (Size) {
  case 0:
  case 1:
    // Implied or accumulator - just opcode
    emitByte(Opcode, CB);
    break;
    
  case 2: {
    // Direct Page or relative - opcode + 1 byte operand
    emitByte(Opcode, CB);
    if (MI.getNumOperands() > 0) {
      const MCOperand &MO = MI.getOperand(MI.getNumOperands() - 1);
      if (MO.isImm()) {
        emitByte(static_cast<uint8_t>(MO.getImm()), CB);
      } else if (MO.isExpr()) {
        // Add fixup for the operand
        Fixups.push_back(MCFixup::create(1, MO.getExpr(), 
                                         MCFixupKind(FK_Data_1)));
        emitByte(0, CB);
      } else {
        emitByte(0, CB);
      }
    } else {
      emitByte(0, CB);
    }
    break;
  }
    
  case 3: {
    // Absolute 16-bit - opcode + 2 byte address
    emitByte(Opcode, CB);
    if (MI.getNumOperands() > 0) {
      const MCOperand &MO = MI.getOperand(MI.getNumOperands() - 1);
      if (MO.isImm()) {
        emitLE16(static_cast<uint16_t>(MO.getImm()), CB);
      } else if (MO.isExpr()) {
        Fixups.push_back(MCFixup::create(1, MO.getExpr(),
                                         MCFixupKind(FK_Data_2)));
        emitLE16(0, CB);
      } else {
        emitLE16(0, CB);
      }
    } else {
      emitLE16(0, CB);
    }
    break;
  }
    
  case 5: {
    // Could be:
    // - WID prefix + opcode + 32-bit immediate ($42 op imm32)
    // - Extended instruction ($02 $E8/E9/EA ...)
    unsigned MIOp = MI.getOpcode();
    
    // Check if this is an extended instruction (FE8_DP, FE9, FEA)
    if (MIOp >= M65832::ADDR_DP && MIOp <= M65832::POPCNT) {
      // Extended instruction format
      emitByte(0x02, CB);  // Extended prefix
      
      // Determine which extended opcode group
      if (MIOp >= M65832::SHLR && MIOp <= M65832::SARR_VAR) {
        // Barrel shifter: $02 $E9 [op|cnt] dest src
        emitByte(0xE9, CB);
        // For constant shifts, encode op|cnt
        uint8_t opcnt = Opcode;
        if (MI.getNumOperands() >= 3 && MI.getOperand(2).isImm()) {
          opcnt = (Opcode & 0xE0) | (MI.getOperand(2).getImm() & 0x1F);
        }
        emitByte(opcnt, CB);
        // dest (register number)
        if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg()) {
          unsigned Reg = MI.getOperand(0).getReg();
          emitByte((Reg - M65832::R0) & 0x3F, CB);
        } else {
          emitByte(0, CB);
        }
        // src (register number)
        if (MI.getNumOperands() >= 2 && MI.getOperand(1).isReg()) {
          unsigned Reg = MI.getOperand(1).getReg();
          emitByte((Reg - M65832::R0) & 0x3F, CB);
        } else {
          emitByte(0, CB);
        }
      } else if (MIOp >= M65832::SEXT8 && MIOp <= M65832::POPCNT) {
        // Extend ops: $02 $EA subop dest src
        emitByte(0xEA, CB);
        emitByte(Opcode, CB);
        // dest
        if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg()) {
          unsigned Reg = MI.getOperand(0).getReg();
          emitByte((Reg - M65832::R0) & 0x3F, CB);
        } else {
          emitByte(0, CB);
        }
        // src
        if (MI.getNumOperands() >= 2 && MI.getOperand(1).isReg()) {
          unsigned Reg = MI.getOperand(1).getReg();
          emitByte((Reg - M65832::R0) & 0x3F, CB);
        } else {
          emitByte(0, CB);
        }
      } else {
        // Register-targeted ALU: $02 $E8 op|mode dest src
        emitByte(0xE8, CB);
        emitByte(Opcode, CB);
        // dest (DP offset = reg * 4)
        if (MI.getNumOperands() >= 1 && MI.getOperand(0).isReg()) {
          unsigned Reg = MI.getOperand(0).getReg();
          emitByte((Reg - M65832::R0) * 4, CB);
        } else {
          emitByte(0, CB);
        }
        // src (DP offset for _DP variants)
        if (MI.getNumOperands() >= 2) {
          const MCOperand &SrcOp = MI.getOperand(MI.getNumOperands() - 1);
          if (SrcOp.isReg()) {
            unsigned Reg = SrcOp.getReg();
            emitByte((Reg - M65832::R0) * 4, CB);
          } else if (SrcOp.isImm()) {
            emitByte(static_cast<uint8_t>(SrcOp.getImm()), CB);
          } else {
            emitByte(0, CB);
          }
        } else {
          emitByte(0, CB);
        }
      }
    } else {
      // Standard WID prefix + opcode + 32-bit immediate
      emitByte(WID_PREFIX, CB);
      emitByte(Opcode, CB);
      if (MI.getNumOperands() > 0) {
        const MCOperand &MO = MI.getOperand(MI.getNumOperands() - 1);
        if (MO.isImm()) {
          emitLE32(static_cast<uint32_t>(MO.getImm()), CB);
        } else if (MO.isExpr()) {
          Fixups.push_back(MCFixup::create(2, MO.getExpr(),
                                           MCFixupKind(FK_Data_4)));
          emitLE32(0, CB);
        } else {
          emitLE32(0, CB);
        }
      } else {
        emitLE32(0, CB);
      }
    }
    break;
  }
    
  case 6: {
    // WID prefix + opcode + 32-bit address
    emitByte(WID_PREFIX, CB);
    emitByte(Opcode, CB);
    if (MI.getNumOperands() > 0) {
      const MCOperand &MO = MI.getOperand(MI.getNumOperands() - 1);
      if (MO.isImm()) {
        emitLE32(static_cast<uint32_t>(MO.getImm()), CB);
      } else if (MO.isExpr()) {
        Fixups.push_back(MCFixup::create(2, MO.getExpr(),
                                         MCFixupKind(FK_Data_4)));
        emitLE32(0, CB);
      } else {
        emitLE32(0, CB);
      }
    } else {
      emitLE32(0, CB);
    }
    break;
  }
    
  default:
    // Unknown size - emit NOPs
    for (unsigned i = 0; i < Size; ++i) {
      emitByte(0xEA, CB);
    }
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
