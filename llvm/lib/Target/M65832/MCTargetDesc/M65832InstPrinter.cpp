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

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

// Include the auto-generated portion of the assembly writer.
#define PRINT_ALIAS_INSTR
#include "M65832GenAsmWriter.inc"

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
    O << '#' << Op.getImm();
  } else if (Op.isExpr()) {
    MAI.printExpr(O, *Op.getExpr());
  } else {
    llvm_unreachable("Unknown operand");
  }
}

void M65832InstPrinter::printDPOperand(const MCInst *MI, unsigned OpNo,
                                         raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isImm()) {
    // Direct Page address - print as hex byte
    O << '$' << format_hex_no_prefix(Op.getImm() & 0xFF, 2);
  } else if (Op.isReg()) {
    // Register number * 4 for DP offset
    unsigned RegNo = Op.getReg();
    O << '$' << format_hex_no_prefix((RegNo & 0x3F) * 4, 2);
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
    O << '+' << Offset.getImm();
  }
}

void M65832InstPrinter::printBranchTarget(const MCInst *MI, unsigned OpNo,
                                            raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isImm()) {
    O << Op.getImm();
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
