//===-- M65832InstPrinter.cpp - Convert M65832 MCInst to asm syntax -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This class prints an M65832 MCInst to assembly.
//
//===----------------------------------------------------------------------===//

#include "M65832InstPrinter.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include <algorithm>

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

// Include the auto-generated portion of the assembly writer.
#define PRINT_ALIAS_INSTR
#include "M65832GenAsmWriter.inc"

static unsigned getHexWidth(uint64_t Value) {
  if (Value <= 0xFF)
    return 2;
  if (Value <= 0xFFFF)
    return 4;
  if (Value <= 0xFFFFFF)
    return 6;
  return 8;
}

static void printHexImm(raw_ostream &O, int64_t Imm,
                        unsigned MinWidth = 0) {
  if (Imm < 0) {
    O << '-';
    Imm = -Imm;
  }
  uint64_t Val = static_cast<uint64_t>(Imm);
  unsigned Width = std::max(MinWidth, getHexWidth(Val));
  O << '$' << format_hex_no_prefix(Val, Width);
}

void M65832InstPrinter::printRegName(raw_ostream &O, MCRegister Reg) {
  O << getRegisterName(Reg);
}

void M65832InstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                    StringRef Annot, const MCSubtargetInfo &STI,
                                    raw_ostream &O) {
  if (!printAliasInstr(MI, Address, O))
    printInstruction(MI, Address, O);
  printAnnotation(O, Annot);
}

void M65832InstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                       raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg()) {
    printRegName(O, Op.getReg());
  } else if (Op.isImm()) {
    // Print immediate value (# prefix is in assembly string if needed)
    printHexImm(O, Op.getImm());
  } else if (Op.isExpr()) {
    MAI.printExpr(O, *Op.getExpr());
  } else {
    llvm_unreachable("Unknown operand");
  }
}

void M65832InstPrinter::printAbsAddr(const MCInst *MI, unsigned OpNo,
                                       raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isImm()) {
    // 32-bit absolute address - print as $XXXXXXXX
    O << '$' << format_hex_no_prefix(Op.getImm() & 0xFFFFFFFF, 8);
  } else if (Op.isExpr()) {
    MAI.printExpr(O, *Op.getExpr());
  }
}

void M65832InstPrinter::printBankRelAddr(const MCInst *MI, unsigned OpNo,
                                           raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isImm()) {
    // Bank-relative address - print as B+$XXXX
    O << "B+$" << format_hex_no_prefix(Op.getImm() & 0xFFFF, 4);
  } else if (Op.isExpr()) {
    O << "B+";
    MAI.printExpr(O, *Op.getExpr());
  }
}

void M65832InstPrinter::printDPOperand(const MCInst *MI, unsigned OpNo,
                                         raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isImm()) {
    // Direct Page address - check if it maps to a register (R0-R63)
    int64_t Addr = Op.getImm() & 0xFF;
    if ((Addr & 0x3) == 0 && Addr < 256) {
      // Aligned to 4 bytes, likely a register - print as Rn
      unsigned RegNum = Addr / 4;
      if (RegNum < 64) {
        O << 'R' << RegNum;
        return;
      }
    }
    // Not a register, print as hex
    O << '$' << format_hex_no_prefix(Addr, 2);
  } else if (Op.isReg()) {
    // Already a register - print as Rn
    unsigned RegNo = Op.getReg();
    // Assuming registers are M65832::R0, R1, etc.
    O << getRegisterName(RegNo);
  } else if (Op.isExpr()) {
    MAI.printExpr(O, *Op.getExpr());
  }
}

void M65832InstPrinter::printMemOperand(const MCInst *MI, unsigned OpNo,
                                          raw_ostream &O) {
  const MCOperand &Base = MI->getOperand(OpNo);
  const MCOperand &Offset = MI->getOperand(OpNo + 1);

  if (Base.isReg()) {
    O << '(';
    printRegName(O, Base.getReg());
    O << ')';
  }
  
  if (Offset.isImm() && Offset.getImm() != 0) {
    int64_t Imm = Offset.getImm();
    if (Imm < 0) {
      O << '-';
      printHexImm(O, -Imm);
    } else {
      O << '+';
      printHexImm(O, Imm);
    }
  }
}

void M65832InstPrinter::printBranchTarget(const MCInst *MI, unsigned OpNo,
                                            raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isImm()) {
    // For relative branches, print as *+offset or *-offset
    int64_t Offset = Op.getImm();
    if (Offset >= 0) {
      O << "*+" << Offset;
    } else {
      O << "*" << Offset;  // Negative already has minus sign
    }
  } else if (Op.isExpr()) {
    MAI.printExpr(O, *Op.getExpr());
  }
}

void M65832InstPrinter::printCondCode(const MCInst *MI, unsigned OpNo,
                                        raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isImm()) {
    switch (Op.getImm()) {
    case 0:  O << "eq"; break;
    case 1:  O << "ne"; break;
    case 2:  O << "cs"; break;
    case 3:  O << "cc"; break;
    case 4:  O << "mi"; break;
    case 5:  O << "pl"; break;
    case 6:  O << "vs"; break;
    case 7:  O << "vc"; break;
    default: O << "??"; break;
    }
  }
}
