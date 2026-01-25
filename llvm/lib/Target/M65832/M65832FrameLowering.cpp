//===-- M65832FrameLowering.cpp - M65832 Frame Information ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the M65832 implementation of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#include "M65832FrameLowering.h"
#include "M65832.h"
#include "M65832InstrInfo.h"
#include "M65832MachineFunctionInfo.h"
#include "M65832Subtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

M65832FrameLowering::M65832FrameLowering(const M65832Subtarget &STI)
    : TargetFrameLowering(TargetFrameLowering::StackGrowsDown,
                          /*StackAlignment=*/Align(4),
                          /*LocalAreaOffset=*/0),
      Subtarget(STI) {}

bool M65832FrameLowering::hasFPImpl(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  
  return MFI.hasVarSizedObjects() || MFI.isFrameAddressTaken() ||
         MF.getTarget().Options.DisableFramePointerElim(MF);
}

bool M65832FrameLowering::hasReservedCallFrame(const MachineFunction &MF) const {
  // Reserve call frame if we don't have variable sized objects
  return !MF.getFrameInfo().hasVarSizedObjects();
}

void M65832FrameLowering::emitPrologue(MachineFunction &MF,
                                        MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator MBBI = MBB.begin();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const M65832InstrInfo &TII =
      *static_cast<const M65832InstrInfo *>(Subtarget.getInstrInfo());
  DebugLoc DL;

  if (MBBI != MBB.end())
    DL = MBBI->getDebugLoc();

  uint64_t StackSize = MFI.getStackSize();

  if (StackSize == 0)
    return;

  // Adjust stack pointer: SP = SP - StackSize
  // We need to do this via A since M65832 can't subtract from SP directly
  // TSX; TXA; SEC; SBC #stacksize; TAX; TXS
  
  // For small stack adjustments, we could use PHx instructions multiple times
  // For larger adjustments, use arithmetic
  
  if (StackSize <= 256) {
    // Small adjustment: use TSX/TXA/SBC/TAX/TXS sequence
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TSX), M65832::X);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TXA), M65832::A)
        .addReg(M65832::X);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::SEC));
    BuildMI(MBB, MBBI, DL, TII.get(M65832::SBC_IMM), M65832::A)
        .addReg(M65832::A)
        .addImm(StackSize);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TAX), M65832::X)
        .addReg(M65832::A);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TXS))
        .addReg(M65832::X);
  } else {
    // Larger adjustment - same sequence but with 32-bit immediate
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TSX), M65832::X);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TXA), M65832::A)
        .addReg(M65832::X);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::SEC));
    BuildMI(MBB, MBBI, DL, TII.get(M65832::SBC_IMM), M65832::A)
        .addReg(M65832::A)
        .addImm(StackSize);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TAX), M65832::X)
        .addReg(M65832::A);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TXS))
        .addReg(M65832::X);
  }

  // If using frame pointer, set it up: FP = SP
  if (hasFP(MF)) {
    // TSX; store X to R29 (FP)
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TSX), M65832::X);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TXA), M65832::A)
        .addReg(M65832::X);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::STA_DP))
        .addReg(M65832::A)
        .addImm(M65832InstrInfo::getDPOffset(29)); // R29 = FP
  }
}

void M65832FrameLowering::emitEpilogue(MachineFunction &MF,
                                        MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator MBBI = MBB.getLastNonDebugInstr();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const M65832InstrInfo &TII =
      *static_cast<const M65832InstrInfo *>(Subtarget.getInstrInfo());
  DebugLoc DL;

  if (MBBI != MBB.end())
    DL = MBBI->getDebugLoc();

  uint64_t StackSize = MFI.getStackSize();

  if (StackSize == 0)
    return;

  // If using frame pointer, restore SP from FP first
  if (hasFP(MF)) {
    BuildMI(MBB, MBBI, DL, TII.get(M65832::LDA_DP), M65832::A)
        .addImm(M65832InstrInfo::getDPOffset(29)); // R29 = FP
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TAX), M65832::X)
        .addReg(M65832::A);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TXS))
        .addReg(M65832::X);
  } else {
    // Restore SP: SP = SP + StackSize
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TSX), M65832::X);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TXA), M65832::A)
        .addReg(M65832::X);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::CLC));
    BuildMI(MBB, MBBI, DL, TII.get(M65832::ADC_IMM), M65832::A)
        .addReg(M65832::A)
        .addImm(StackSize);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TAX), M65832::X)
        .addReg(M65832::A);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TXS))
        .addReg(M65832::X);
  }
}

MachineBasicBlock::iterator M65832FrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MI) const {
  // If we have a reserved call frame, these are no-ops
  if (hasReservedCallFrame(MF))
    return MBB.erase(MI);

  // Otherwise, adjust the stack pointer
  const M65832InstrInfo &TII =
      *static_cast<const M65832InstrInfo *>(Subtarget.getInstrInfo());
  DebugLoc DL = MI->getDebugLoc();
  
  int64_t Amount = MI->getOperand(0).getImm();
  
  if (Amount == 0)
    return MBB.erase(MI);
  
  if (MI->getOpcode() == M65832::ADJCALLSTACKDOWN) {
    // Subtract from SP
    BuildMI(MBB, MI, DL, TII.get(M65832::TSX), M65832::X);
    BuildMI(MBB, MI, DL, TII.get(M65832::TXA), M65832::A)
        .addReg(M65832::X);
    BuildMI(MBB, MI, DL, TII.get(M65832::SEC));
    BuildMI(MBB, MI, DL, TII.get(M65832::SBC_IMM), M65832::A)
        .addReg(M65832::A)
        .addImm(Amount);
    BuildMI(MBB, MI, DL, TII.get(M65832::TAX), M65832::X)
        .addReg(M65832::A);
    BuildMI(MBB, MI, DL, TII.get(M65832::TXS))
        .addReg(M65832::X);
  } else {
    // ADJCALLSTACKUP - add to SP
    BuildMI(MBB, MI, DL, TII.get(M65832::TSX), M65832::X);
    BuildMI(MBB, MI, DL, TII.get(M65832::TXA), M65832::A)
        .addReg(M65832::X);
    BuildMI(MBB, MI, DL, TII.get(M65832::CLC));
    BuildMI(MBB, MI, DL, TII.get(M65832::ADC_IMM), M65832::A)
        .addReg(M65832::A)
        .addImm(Amount);
    BuildMI(MBB, MI, DL, TII.get(M65832::TAX), M65832::X)
        .addReg(M65832::A);
    BuildMI(MBB, MI, DL, TII.get(M65832::TXS))
        .addReg(M65832::X);
  }
  
  return MBB.erase(MI);
}

void M65832FrameLowering::determineCalleeSaves(MachineFunction &MF,
                                                BitVector &SavedRegs,
                                                RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);
  
  // If using frame pointer, it must be saved
  if (hasFP(MF))
    SavedRegs.set(M65832::R29);
}

StackOffset
M65832FrameLowering::getFrameIndexReference(const MachineFunction &MF, int FI,
                                             Register &FrameReg) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  
  // Calculate offset from frame register
  int64_t Offset = MFI.getObjectOffset(FI);
  
  if (hasFP(MF)) {
    FrameReg = M65832::R29;
    // FP points to the saved FP location
  } else {
    FrameReg = M65832::SP;
    Offset += MFI.getStackSize();
  }
  
  return StackOffset::getFixed(Offset);
}

bool M65832FrameLowering::spillCalleeSavedRegisters(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
    ArrayRef<CalleeSavedInfo> CSI, const TargetRegisterInfo *TRI) const {
  if (CSI.empty())
    return true;

  const M65832InstrInfo &TII =
      *static_cast<const M65832InstrInfo *>(Subtarget.getInstrInfo());
  DebugLoc DL;
  if (MI != MBB.end())
    DL = MI->getDebugLoc();

  // Save each callee-saved register by pushing to stack
  // For M65832, we save GPRs via: LDA $dp; PHA
  for (const CalleeSavedInfo &Info : CSI) {
    Register Reg = Info.getReg();
    
    // Get the DP offset for this register
    unsigned RegNum = Reg - M65832::R0;
    unsigned DPOffset = M65832InstrInfo::getDPOffset(RegNum);
    
    // LDA from DP location, then push
    BuildMI(MBB, MI, DL, TII.get(M65832::LDA_DP), M65832::A)
        .addImm(DPOffset);
    BuildMI(MBB, MI, DL, TII.get(M65832::PHA))
        .addReg(M65832::A, RegState::Kill);
  }
  
  return true;
}

bool M65832FrameLowering::restoreCalleeSavedRegisters(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
    MutableArrayRef<CalleeSavedInfo> CSI, const TargetRegisterInfo *TRI) const {
  if (CSI.empty())
    return true;

  const M65832InstrInfo &TII =
      *static_cast<const M65832InstrInfo *>(Subtarget.getInstrInfo());
  DebugLoc DL;
  if (MI != MBB.end())
    DL = MI->getDebugLoc();

  // Restore each callee-saved register by popping from stack (reverse order)
  for (auto I = CSI.rbegin(), E = CSI.rend(); I != E; ++I) {
    Register Reg = I->getReg();
    
    // Get the DP offset for this register
    unsigned RegNum = Reg - M65832::R0;
    unsigned DPOffset = M65832InstrInfo::getDPOffset(RegNum);
    
    // Pop into A, then store to DP location
    BuildMI(MBB, MI, DL, TII.get(M65832::PLA), M65832::A);
    BuildMI(MBB, MI, DL, TII.get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DPOffset);
  }
  
  return true;
}
