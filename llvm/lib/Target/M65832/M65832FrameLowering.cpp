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
  bool UseFP = hasFP(MF);

  // If using frame pointer, save old FP first, then set up new FP
  // This must happen BEFORE stack allocation so RTS can find return address
  if (UseFP) {
    // Push old FP value to stack
    BuildMI(MBB, MBBI, DL, TII.get(M65832::LDA_DP), M65832::A)
        .addImm(M65832InstrInfo::getDPOffset(29)); // Load old R29
    BuildMI(MBB, MBBI, DL, TII.get(M65832::PHA))
        .addReg(M65832::A, RegState::Kill);
    
    // Set FP = SP (FP now points to saved FP location)
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TSX), M65832::X);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TXA), M65832::A)
        .addReg(M65832::X);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::STA_DP))
        .addReg(M65832::A)
        .addImm(M65832InstrInfo::getDPOffset(29)); // R29 = FP = SP
  }

  if (StackSize == 0)
    return;

  // Adjust stack pointer: SP = SP - StackSize
  // We need to do this via A since M65832 can't subtract from SP directly
  // TSX; TXA; SEC; SBC #stacksize; TAX; TXS
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
  bool UseFP = hasFP(MF);

  if (UseFP) {
    // Restore SP from FP (SP now points to saved FP location on stack)
    BuildMI(MBB, MBBI, DL, TII.get(M65832::LDA_DP), M65832::A)
        .addImm(M65832InstrInfo::getDPOffset(29)); // Load FP
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TAX), M65832::X)
        .addReg(M65832::A);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::TXS))
        .addReg(M65832::X);
    
    // Pop old FP value from stack
    // After this, SP points to return address
    BuildMI(MBB, MBBI, DL, TII.get(M65832::PLA), M65832::A);
    BuildMI(MBB, MBBI, DL, TII.get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(M65832InstrInfo::getDPOffset(29)); // Restore old R29
  } else if (StackSize > 0) {
    // No FP: Restore SP by adding StackSize
    // SP = SP + StackSize puts SP back to where return address is
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
  
  // If using frame pointer, R29 is saved/restored in prologue/epilogue,
  // NOT in the general callee-save mechanism. Remove it from SavedRegs.
  if (hasFP(MF))
    SavedRegs.reset(M65832::R29);
}

StackOffset
M65832FrameLowering::getFrameIndexReference(const MachineFunction &MF, int FI,
                                             Register &FrameReg) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  
  // Calculate offset from frame register
  int64_t Offset = MFI.getObjectOffset(FI);
  
  if (hasFP(MF)) {
    FrameReg = M65832::R29;
    // FP points to the saved FP location on the stack.
    // Local variables are below FP (at negative offsets).
    // The offset from getObjectOffset is relative to the frame base,
    // which is where FP points (the saved FP slot).
    // So we use the offset directly.
    Offset += MFI.getStackSize();
  } else {
    FrameReg = M65832::SP;
    // SP points to the bottom of the stack frame.
    // Need to add StackSize to convert from frame-relative to SP-relative.
    Offset += MFI.getStackSize();
  }
  
  return StackOffset::getFixed(Offset);
}

bool M65832FrameLowering::spillCalleeSavedRegisters(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
    ArrayRef<CalleeSavedInfo> CSI, const TargetRegisterInfo *TRI) const {
  if (CSI.empty())
    return true;

  MachineFunction &MF = *MBB.getParent();
  const M65832InstrInfo &TII =
      *static_cast<const M65832InstrInfo *>(Subtarget.getInstrInfo());
  DebugLoc DL;
  if (MI != MBB.end())
    DL = MI->getDebugLoc();

  // Save each callee-saved register by pushing to stack
  // For M65832, we save GPRs via: LDA $dp; PHA
  // Note: R29 (FP) is handled separately in prologue/epilogue when hasFP
  for (const CalleeSavedInfo &Info : CSI) {
    Register Reg = Info.getReg();
    
    // Skip R29 if using frame pointer - it's handled in prologue/epilogue
    if (Reg == M65832::R29 && hasFP(MF))
      continue;
    
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

  MachineFunction &MF = *MBB.getParent();
  const M65832InstrInfo &TII =
      *static_cast<const M65832InstrInfo *>(Subtarget.getInstrInfo());
  DebugLoc DL;
  if (MI != MBB.end())
    DL = MI->getDebugLoc();

  // Restore each callee-saved register by popping from stack (reverse order)
  // Note: R29 (FP) is handled separately in prologue/epilogue when hasFP
  for (auto I = CSI.rbegin(), E = CSI.rend(); I != E; ++I) {
    Register Reg = I->getReg();
    
    // Skip R29 if using frame pointer - it's handled in epilogue
    if (Reg == M65832::R29 && hasFP(MF))
      continue;
    
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
