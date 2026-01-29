//===-- M65832AsmPrinter.cpp - M65832 LLVM assembly writer ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to M65832 assembly language.
//
//===----------------------------------------------------------------------===//

#include "M65832.h"
#include "M65832InstrInfo.h"
#include "M65832MCInstLower.h"
#include "M65832TargetMachine.h"
#include "MCTargetDesc/M65832InstPrinter.h"
#include "TargetInfo/M65832TargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/MC/MCRegister.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

namespace {
class M65832AsmPrinter : public AsmPrinter {
public:
  explicit M65832AsmPrinter(TargetMachine &TM,
                             std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)) {}

  StringRef getPassName() const override { return "M65832 Assembly Printer"; }

  void emitInstruction(const MachineInstr *MI) override;

  bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                       const char *ExtraCode, raw_ostream &OS) override;

  bool PrintAsmMemoryOperand(const MachineInstr *MI, unsigned OpNo,
                             const char *ExtraCode, raw_ostream &OS) override;
};
} // end anonymous namespace

void M65832AsmPrinter::emitInstruction(const MachineInstr *MI) {
  M65832MCInstLower MCInstLowering(OutContext, *this);
  MCInst TmpInst;
  MCInstLowering.Lower(MI, TmpInst);
  EmitToStreamer(*OutStreamer, TmpInst);
}

bool M65832AsmPrinter::PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                                         const char *ExtraCode,
                                         raw_ostream &OS) {
  if (ExtraCode && ExtraCode[0])
    return true; // Unknown modifier

  const MachineOperand &MO = MI->getOperand(OpNo);
  switch (MO.getType()) {
  case MachineOperand::MO_Register:
    OS << M65832InstPrinter::getRegisterName(MO.getReg());
    return false;
  case MachineOperand::MO_Immediate:
    OS << MO.getImm();
    return false;
  case MachineOperand::MO_GlobalAddress:
    PrintSymbolOperand(MO, OS);
    return false;
  default:
    return true;
  }
}

bool M65832AsmPrinter::PrintAsmMemoryOperand(const MachineInstr *MI,
                                               unsigned OpNo,
                                               const char *ExtraCode,
                                               raw_ostream &OS) {
  if (ExtraCode && ExtraCode[0])
    return true; // Unknown modifier

  // Bounds check
  if (OpNo >= MI->getNumOperands())
    return true;
    
  const MachineOperand &MO = MI->getOperand(OpNo);
  
  // Memory operand is a register containing the address
  if (MO.isReg()) {
    Register Reg = MO.getReg();
    // Check if valid register (non-zero ID)
    if (Reg.id() != 0) {
      OS << M65832InstPrinter::getRegisterName(Reg);
      return false;
    }
    // NoReg - shouldn't happen, but handle gracefully
    OS << "R0";
    return false;
  }
  
  if (MO.isGlobal()) {
    // Global address - print as B+symbol
    OS << "B+";
    PrintSymbolOperand(MO, OS);
    return false;
  }
  
  if (MO.isImm()) {
    // Immediate address - print as hex
    OS << '$' << format_hex_no_prefix(MO.getImm(), 8);
    return false;
  }
  
  // Unknown operand type
  return true;
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeM65832AsmPrinter() {
  RegisterAsmPrinter<M65832AsmPrinter> X(getTheM65832Target());
}
