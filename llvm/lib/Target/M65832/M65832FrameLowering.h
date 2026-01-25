//===-- M65832FrameLowering.h - Define frame lowering for M65832 -*- C++ -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This class implements M65832-specific bits of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M65832_M65832FRAMELOWERING_H
#define LLVM_LIB_TARGET_M65832_M65832FRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {

class M65832Subtarget;

class M65832FrameLowering : public TargetFrameLowering {
  const M65832Subtarget &Subtarget;

public:
  explicit M65832FrameLowering(const M65832Subtarget &STI);

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  bool hasFPImpl(const MachineFunction &MF) const override;
  bool hasReservedCallFrame(const MachineFunction &MF) const override;

  MachineBasicBlock::iterator
  eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator MI) const override;

  void determineCalleeSaves(MachineFunction &MF, BitVector &SavedRegs,
                            RegScavenger *RS = nullptr) const override;

  StackOffset getFrameIndexReference(const MachineFunction &MF, int FI,
                                     Register &FrameReg) const override;

  bool spillCalleeSavedRegisters(MachineBasicBlock &MBB,
                                  MachineBasicBlock::iterator MI,
                                  ArrayRef<CalleeSavedInfo> CSI,
                                  const TargetRegisterInfo *TRI) const override;

  bool restoreCalleeSavedRegisters(MachineBasicBlock &MBB,
                                    MachineBasicBlock::iterator MI,
                                    MutableArrayRef<CalleeSavedInfo> CSI,
                                    const TargetRegisterInfo *TRI) const override;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_M65832_M65832FRAMELOWERING_H
