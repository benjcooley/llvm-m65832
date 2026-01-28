//===-- M65832RegisterInfo.cpp - M65832 Register Information --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the M65832 implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#include "M65832RegisterInfo.h"
#include "M65832.h"
#include "M65832FrameLowering.h"
#include "M65832Subtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "m65832-reginfo"

#define GET_REGINFO_TARGET_DESC
#include "M65832GenRegisterInfo.inc"

M65832RegisterInfo::M65832RegisterInfo(const M65832Subtarget & /*STI*/)
    : M65832GenRegisterInfo(M65832::R30) {} // Return address register

const MCPhysReg *
M65832RegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR_M65832_SaveList;
}

const uint32_t *
M65832RegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                          CallingConv::ID CC) const {
  return CSR_M65832_RegMask;
}

BitVector M65832RegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());

  // Stack pointer is always reserved
  Reserved.set(M65832::SP);
  
  // Base registers are reserved
  Reserved.set(M65832::D);
  Reserved.set(M65832::B);
  Reserved.set(M65832::VBR);
  
  // Temp register is reserved
  Reserved.set(M65832::T);
  
  // Status register
  Reserved.set(M65832::SR);
  
  // Kernel reserved registers
  Reserved.set(M65832::R24);
  Reserved.set(M65832::R25);
  Reserved.set(M65832::R26);
  Reserved.set(M65832::R27);
  
  // Global pointer
  Reserved.set(M65832::R28);
  
  // Reserved R31
  Reserved.set(M65832::R31);
  
  // Future reserved registers
  Reserved.set(M65832::R56);
  Reserved.set(M65832::R57);
  Reserved.set(M65832::R58);
  Reserved.set(M65832::R59);
  Reserved.set(M65832::R60);
  Reserved.set(M65832::R61);
  Reserved.set(M65832::R62);
  Reserved.set(M65832::R63);

  // Frame pointer if used
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  if (MFI.hasVarSizedObjects() || MFI.isFrameAddressTaken()) {
    Reserved.set(M65832::R29);
  }

  return Reserved;
}

bool M65832RegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                               int SPAdj, unsigned FIOperandNum,
                                               RegScavenger *RS) const {
  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  int64_t Offset = MFI.getObjectOffset(FrameIndex);
  
  // Add the stack size to get offset from SP
  Offset += MFI.getStackSize();
  Offset += SPAdj;
  
  // Check if there's an additional offset operand after the frame index
  // This is the case for complex memory operands like memsrc
  unsigned NumOps = MI.getNumOperands();
  if (FIOperandNum + 1 < NumOps) {
    MachineOperand &OffsetOp = MI.getOperand(FIOperandNum + 1);
    if (OffsetOp.isImm()) {
      Offset += OffsetOp.getImm();
      // Update the offset operand with the final computed offset
      OffsetOp.setImm(Offset);
    }
  }

  // Get the frame register (SP or FP)
  Register FrameReg = getFrameRegister(MF);

  // Replace the frame index with the frame register
  // The load/store pseudo expansion will handle the actual SP+offset computation
  MI.getOperand(FIOperandNum).ChangeToRegister(FrameReg, false);

  return false;
}

Register M65832RegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  // Use B as the frame base for locals/stack addressing
  return M65832::B;
}
