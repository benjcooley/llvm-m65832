//===-- M65832InstrInfo.cpp - M65832 Instruction Information --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the M65832 implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "M65832InstrInfo.h"
#include "M65832.h"
#include "M65832Subtarget.h"
#include "M65832TargetMachine.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "m65832-instr-info"

#define GET_INSTRINFO_CTOR_DTOR
#include "M65832GenInstrInfo.inc"

M65832InstrInfo::M65832InstrInfo(const M65832Subtarget &STI)
    : M65832GenInstrInfo(STI, RI, M65832::ADJCALLSTACKDOWN,
                          M65832::ADJCALLSTACKUP),
      RI(STI) {}

void M65832InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator I,
                                   const DebugLoc &DL, Register DestReg,
                                   Register SrcReg, bool KillSrc,
                                   bool RenamableDest, bool RenamableSrc) const {
  // For GPR to GPR copy, we use: LDA src_dp; STA dst_dp
  // This goes through the accumulator A
  
  if (M65832::GPRRegClass.contains(DestReg) &&
      M65832::GPRRegClass.contains(SrcReg)) {
    // Register-to-register move via Extended ALU (LD.L)
    BuildMI(MBB, I, DL, get(M65832::MOVR_DP), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
    return;
  }
  
  // Copy to/from accumulator
  if (DestReg == M65832::A && M65832::GPRRegClass.contains(SrcReg)) {
    unsigned SrcDP = getDPOffset(SrcReg - M65832::R0);
    BuildMI(MBB, I, DL, get(M65832::LDA_DP), M65832::A)
        .addImm(SrcDP);
    return;
  }
  
  if (M65832::GPRRegClass.contains(DestReg) && SrcReg == M65832::A) {
    unsigned DstDP = getDPOffset(DestReg - M65832::R0);
    BuildMI(MBB, I, DL, get(M65832::STA_DP))
        .addReg(M65832::A, getKillRegState(KillSrc))
        .addImm(DstDP);
    return;
  }
  
  // A <-> X transfers
  if (DestReg == M65832::X && SrcReg == M65832::A) {
    BuildMI(MBB, I, DL, get(M65832::TAX), M65832::X)
        .addReg(M65832::A, getKillRegState(KillSrc));
    return;
  }
  
  if (DestReg == M65832::A && SrcReg == M65832::X) {
    BuildMI(MBB, I, DL, get(M65832::TXA), M65832::A)
        .addReg(M65832::X, getKillRegState(KillSrc));
    return;
  }
  
  // A <-> Y transfers
  if (DestReg == M65832::Y && SrcReg == M65832::A) {
    BuildMI(MBB, I, DL, get(M65832::TAY), M65832::Y)
        .addReg(M65832::A, getKillRegState(KillSrc));
    return;
  }
  
  if (DestReg == M65832::A && SrcReg == M65832::Y) {
    BuildMI(MBB, I, DL, get(M65832::TYA), M65832::A)
        .addReg(M65832::Y, getKillRegState(KillSrc));
    return;
  }
  
  // GPR <-> Y transfers (via A)
  if (DestReg == M65832::Y && M65832::GPRRegClass.contains(SrcReg)) {
    unsigned SrcDP = getDPOffset(SrcReg - M65832::R0);
    BuildMI(MBB, I, DL, get(M65832::LDA_DP), M65832::A).addImm(SrcDP);
    BuildMI(MBB, I, DL, get(M65832::TAY), M65832::Y)
        .addReg(M65832::A, RegState::Kill);
    return;
  }
  
  if (M65832::GPRRegClass.contains(DestReg) && SrcReg == M65832::Y) {
    unsigned DstDP = getDPOffset(DestReg - M65832::R0);
    BuildMI(MBB, I, DL, get(M65832::TYA), M65832::A).addReg(M65832::Y, getKillRegState(KillSrc));
    BuildMI(MBB, I, DL, get(M65832::STA_DP)).addReg(M65832::A, RegState::Kill).addImm(DstDP);
    return;
  }
  
  // GPR <-> X transfers (via A)
  if (DestReg == M65832::X && M65832::GPRRegClass.contains(SrcReg)) {
    unsigned SrcDP = getDPOffset(SrcReg - M65832::R0);
    BuildMI(MBB, I, DL, get(M65832::LDA_DP), M65832::A).addImm(SrcDP);
    BuildMI(MBB, I, DL, get(M65832::TAX), M65832::X)
        .addReg(M65832::A, RegState::Kill);
    return;
  }
  
  if (M65832::GPRRegClass.contains(DestReg) && SrcReg == M65832::X) {
    unsigned DstDP = getDPOffset(DestReg - M65832::R0);
    BuildMI(MBB, I, DL, get(M65832::TXA), M65832::A).addReg(M65832::X, getKillRegState(KillSrc));
    BuildMI(MBB, I, DL, get(M65832::STA_DP)).addReg(M65832::A, RegState::Kill).addImm(DstDP);
    return;
  }
  
  // X <-> Y transfers (via A)
  if (DestReg == M65832::Y && SrcReg == M65832::X) {
    BuildMI(MBB, I, DL, get(M65832::TXA), M65832::A).addReg(M65832::X, getKillRegState(KillSrc));
    BuildMI(MBB, I, DL, get(M65832::TAY), M65832::Y).addReg(M65832::A, RegState::Kill);
    return;
  }
  
  if (DestReg == M65832::X && SrcReg == M65832::Y) {
    BuildMI(MBB, I, DL, get(M65832::TYA), M65832::A).addReg(M65832::Y, getKillRegState(KillSrc));
    BuildMI(MBB, I, DL, get(M65832::TAX), M65832::X).addReg(M65832::A, RegState::Kill);
    return;
  }
  
  // SP <-> GPR transfers
  if (M65832::GPRRegClass.contains(DestReg) && SrcReg == M65832::SP) {
    // TSX; TXA; STA dst  (copy SP to GPR via X and A)
    unsigned DstDP = getDPOffset(DestReg - M65832::R0);
    BuildMI(MBB, I, DL, get(M65832::TSX), M65832::X);
    BuildMI(MBB, I, DL, get(M65832::TXA), M65832::A).addReg(M65832::X, RegState::Kill);
    BuildMI(MBB, I, DL, get(M65832::STA_DP)).addReg(M65832::A, RegState::Kill).addImm(DstDP);
    return;
  }
  
  if (DestReg == M65832::SP && M65832::GPRRegClass.contains(SrcReg)) {
    // LDA src; TAX; TXS  (copy GPR to SP via A and X)
    unsigned SrcDP = getDPOffset(SrcReg - M65832::R0);
    BuildMI(MBB, I, DL, get(M65832::LDA_DP), M65832::A).addImm(SrcDP);
    BuildMI(MBB, I, DL, get(M65832::TAX), M65832::X).addReg(M65832::A, RegState::Kill);
    BuildMI(MBB, I, DL, get(M65832::TXS)).addReg(M65832::X, RegState::Kill);
    return;
  }
  
  // FPU register copies
  if (M65832::FPR32RegClass.contains(DestReg) && M65832::FPR32RegClass.contains(SrcReg)) {
    BuildMI(MBB, I, DL, get(M65832::FMOV_S), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
    return;
  }
  
  if (M65832::FPR64RegClass.contains(DestReg) && M65832::FPR64RegClass.contains(SrcReg)) {
    BuildMI(MBB, I, DL, get(M65832::FMOV_D), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
    return;
  }
  
  // Print debug info before crashing
  dbgs() << "Cannot copy from " << printReg(SrcReg, &getRegisterInfo())
         << " to " << printReg(DestReg, &getRegisterInfo()) << "\n";
  llvm_unreachable("Cannot copy between these registers");
}

void M65832InstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                           MachineBasicBlock::iterator I,
                                           Register SrcReg, bool isKill,
                                           int FrameIndex,
                                           const TargetRegisterClass *RC,
                                           Register VReg,
                                           MachineInstr::MIFlag Flags) const {
  DebugLoc DL;
  if (I != MBB.end())
    DL = I->getDebugLoc();

  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();

  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIndex),
      MachineMemOperand::MOStore, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlign(FrameIndex));

  if (M65832::GPRRegClass.hasSubClassEq(RC)) {
    // Use STORE32 pseudo which properly supports frame indices
    // STORE32 has mayStore=1 and uses memsrc operand (base + offset)
    BuildMI(MBB, I, DL, get(M65832::STORE32))
        .addReg(SrcReg, getKillRegState(isKill))
        .addFrameIndex(FrameIndex)
        .addImm(0)
        .addMemOperand(MMO);
  } else if (RC == &M65832::ACCRegClass) {
    BuildMI(MBB, I, DL, get(M65832::PHA))
        .addReg(SrcReg, getKillRegState(isKill));
  } else if (M65832::FPR32RegClass.hasSubClassEq(RC)) {
    // Use STF32 pseudo for 32-bit floating point
    BuildMI(MBB, I, DL, get(M65832::STF32))
        .addReg(SrcReg, getKillRegState(isKill))
        .addFrameIndex(FrameIndex)
        .addImm(0)
        .addMemOperand(MMO);
  } else if (M65832::FPR64RegClass.hasSubClassEq(RC)) {
    // Use STF64 pseudo for 64-bit floating point
    BuildMI(MBB, I, DL, get(M65832::STF64))
        .addReg(SrcReg, getKillRegState(isKill))
        .addFrameIndex(FrameIndex)
        .addImm(0)
        .addMemOperand(MMO);
  } else {
    llvm_unreachable("Cannot store this register class to stack slot");
  }
}

void M65832InstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                            MachineBasicBlock::iterator I,
                                            Register DestReg, int FrameIndex,
                                            const TargetRegisterClass *RC,
                                            Register VReg, unsigned SubReg,
                                            MachineInstr::MIFlag Flags) const {
  DebugLoc DL;
  if (I != MBB.end())
    DL = I->getDebugLoc();

  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();

  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIndex),
      MachineMemOperand::MOLoad, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlign(FrameIndex));

  if (M65832::GPRRegClass.hasSubClassEq(RC)) {
    // Use LOAD32 pseudo which properly supports frame indices
    // LOAD32 has mayLoad=1 and uses memsrc operand (base + offset)
    BuildMI(MBB, I, DL, get(M65832::LOAD32), DestReg)
        .addFrameIndex(FrameIndex)
        .addImm(0)
        .addMemOperand(MMO);
  } else if (RC == &M65832::ACCRegClass) {
    BuildMI(MBB, I, DL, get(M65832::PLA), DestReg);
  } else if (M65832::FPR32RegClass.hasSubClassEq(RC)) {
    // Use LDF32 pseudo for 32-bit floating point
    BuildMI(MBB, I, DL, get(M65832::LDF32), DestReg)
        .addFrameIndex(FrameIndex)
        .addImm(0)
        .addMemOperand(MMO);
  } else if (M65832::FPR64RegClass.hasSubClassEq(RC)) {
    // Use LDF64 pseudo for 64-bit floating point
    BuildMI(MBB, I, DL, get(M65832::LDF64), DestReg)
        .addFrameIndex(FrameIndex)
        .addImm(0)
        .addMemOperand(MMO);
  } else {
    llvm_unreachable("Cannot load this register class from stack slot");
  }
}

bool M65832InstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                     MachineBasicBlock *&TBB,
                                     MachineBasicBlock *&FBB,
                                     SmallVectorImpl<MachineOperand> &Cond,
                                     bool AllowModify) const {
  // Start from the bottom of the block and work up
  MachineBasicBlock::iterator I = MBB.end();
  while (I != MBB.begin()) {
    --I;
    
    if (I->isDebugInstr())
      continue;
    
    // If we've hit a non-terminator, we're done
    if (!isUnpredicatedTerminator(*I))
      break;
    
    // Handle unconditional branches
    if (I->getOpcode() == M65832::BRA || I->getOpcode() == M65832::JMP) {
      // Check if operand is actually an MBB - might be immediate for inline asm
      if (!I->getOperand(0).isMBB())
        return true; // Can't analyze non-MBB branch targets
      if (TBB) {
        // Already have a branch target - this is the fallthrough
        FBB = TBB;
      }
      TBB = I->getOperand(0).getMBB();
      continue;
    }
    
    // Handle conditional branches
    if (I->getOpcode() == M65832::BEQ || I->getOpcode() == M65832::BNE ||
        I->getOpcode() == M65832::BCS || I->getOpcode() == M65832::BCC ||
        I->getOpcode() == M65832::BMI || I->getOpcode() == M65832::BPL ||
        I->getOpcode() == M65832::BVS || I->getOpcode() == M65832::BVC) {
      // Check if operand is actually an MBB - might be immediate for inline asm
      if (!I->getOperand(0).isMBB())
        return true; // Can't analyze non-MBB branch targets
      if (TBB) {
        // Already have unconditional, this is conditional to different target
        return true; // Can't handle this
      }
      TBB = I->getOperand(0).getMBB();
      Cond.push_back(MachineOperand::CreateImm(I->getOpcode()));
      continue;
    }
    
    // Unknown terminator
    return true;
  }
  
  return false;
}

unsigned M65832InstrInfo::removeBranch(MachineBasicBlock &MBB,
                                        int *BytesRemoved) const {
  MachineBasicBlock::iterator I = MBB.end();
  unsigned Count = 0;
  int Removed = 0;

  while (I != MBB.begin()) {
    --I;
    if (I->isDebugInstr())
      continue;
    if (!I->isBranch())
      break;
    
    Removed += I->getDesc().getSize();
    I->eraseFromParent();
    I = MBB.end();
    ++Count;
  }

  if (BytesRemoved)
    *BytesRemoved = Removed;
  return Count;
}

unsigned M65832InstrInfo::insertBranch(MachineBasicBlock &MBB,
                                        MachineBasicBlock *TBB,
                                        MachineBasicBlock *FBB,
                                        ArrayRef<MachineOperand> Cond,
                                        const DebugLoc &DL,
                                        int *BytesAdded) const {
  unsigned Count = 0;
  int Added = 0;

  if (Cond.empty()) {
    // Unconditional branch
    MachineInstr &MI = *BuildMI(&MBB, DL, get(M65832::BRA)).addMBB(TBB);
    Added += MI.getDesc().getSize();
    ++Count;
  } else {
    // Conditional branch
    unsigned Opc = Cond[0].getImm();
    MachineInstr &MI = *BuildMI(&MBB, DL, get(Opc)).addMBB(TBB);
    Added += MI.getDesc().getSize();
    ++Count;

    if (FBB) {
      // Need unconditional branch to false block
      MachineInstr &MI2 = *BuildMI(&MBB, DL, get(M65832::BRA)).addMBB(FBB);
      Added += MI2.getDesc().getSize();
      ++Count;
    }
  }

  if (BytesAdded)
    *BytesAdded = Added;
  return Count;
}

bool M65832InstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  if (Cond.size() != 1)
    return true;

  unsigned Opc = Cond[0].getImm();
  unsigned NewOpc;

  switch (Opc) {
  case M65832::BEQ: NewOpc = M65832::BNE; break;
  case M65832::BNE: NewOpc = M65832::BEQ; break;
  case M65832::BCS: NewOpc = M65832::BCC; break;
  case M65832::BCC: NewOpc = M65832::BCS; break;
  case M65832::BMI: NewOpc = M65832::BPL; break;
  case M65832::BPL: NewOpc = M65832::BMI; break;
  case M65832::BVS: NewOpc = M65832::BVC; break;
  case M65832::BVC: NewOpc = M65832::BVS; break;
  default:
    return true;
  }

  Cond[0].setImm(NewOpc);
  return false;
}

bool M65832InstrInfo::expandPostRAPseudo(MachineInstr &MI) const {
  MachineBasicBlock &MBB = *MI.getParent();
  DebugLoc DL = MI.getDebugLoc();

  switch (MI.getOpcode()) {
  default:
    return false;

  case M65832::LI: {
    // Load immediate: LD.L $dst,#imm
    Register DstReg = MI.getOperand(0).getReg();
    int64_t Imm = MI.getOperand(1).getImm();
    BuildMI(MBB, MI, DL, get(M65832::LDR_IMM), DstReg).addImm(Imm);
    break;
  }

  case M65832::LA:
  case M65832::LA_EXT:
  case M65832::LA_BA:
  case M65832::LA_CP: {
    // Load address: LD.L $dst,#addr
    // LA_CP is for constant pool entries (floating point constants, etc.)
    Register DstReg = MI.getOperand(0).getReg();
    BuildMI(MBB, MI, DL, get(M65832::LDR_IMM), DstReg)
        .add(MI.getOperand(1)); // Copy address operand
    break;
  }

  case M65832::LEA_FI: {
    // Load effective address from frame index
    // After eliminateFrameIndex, operands are: dst, FrameReg, Offset
    Register DstReg = MI.getOperand(0).getReg();
    Register FrameReg = MI.getOperand(1).getReg();
    int64_t Offset = 0;
    if (MI.getNumOperands() > 2 && MI.getOperand(2).isImm()) {
      Offset = MI.getOperand(2).getImm();
    }
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);

    // Compute FrameReg + Offset
    if (FrameReg == M65832::SP) {
      // For SP-relative: TSX; TXA; CLC; ADC #offset; STA dst
      BuildMI(MBB, MI, DL, get(M65832::TSX), M65832::X);
      BuildMI(MBB, MI, DL, get(M65832::TXA), M65832::A).addReg(M65832::X);
      if (Offset != 0) {
        BuildMI(MBB, MI, DL, get(M65832::CLC));
        BuildMI(MBB, MI, DL, get(M65832::ADC_IMM), M65832::A)
            .addReg(M65832::A)
            .addImm(Offset);
      }
      BuildMI(MBB, MI, DL, get(M65832::STA_DP))
          .addReg(M65832::A, RegState::Kill)
          .addImm(DstDP);
    } else if (FrameReg == M65832::B) {
      // For B-relative: Use TBA to transfer B to A directly
      // TBA; CLC; ADC #offset; STA dst
      BuildMI(MBB, MI, DL, get(M65832::TBA), M65832::A);
      if (Offset != 0) {
        BuildMI(MBB, MI, DL, get(M65832::CLC));
        BuildMI(MBB, MI, DL, get(M65832::ADC_IMM), M65832::A)
            .addReg(M65832::A)
            .addImm(Offset);
      }
      BuildMI(MBB, MI, DL, get(M65832::STA_DP))
          .addReg(M65832::A, RegState::Kill)
          .addImm(DstDP);
    } else {
      // LDA FrameReg; CLC; ADC #offset; STA dst
      unsigned FrameDP = getDPOffset(29);  // R29 = FP
      if (FrameReg != M65832::R29) {
        FrameDP = getDPOffset(FrameReg - M65832::R0);
      }
      BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(FrameDP);
      if (Offset != 0) {
        BuildMI(MBB, MI, DL, get(M65832::CLC));
        BuildMI(MBB, MI, DL, get(M65832::ADC_IMM), M65832::A)
            .addReg(M65832::A)
            .addImm(Offset);
      }
      BuildMI(MBB, MI, DL, get(M65832::STA_DP))
          .addReg(M65832::A, RegState::Kill)
          .addImm(DstDP);
    }
    break;
  }

  case M65832::ADD_GPR: {
    // ADD: dst = src1 + src2 using A (destructive)
    Register DstReg = MI.getOperand(0).getReg();
    Register Src1Reg = MI.getOperand(1).getReg();
    Register Src2Reg = MI.getOperand(2).getReg();
    unsigned Src1DP = getDPOffset(Src1Reg - M65832::R0);
    unsigned Src2DP = getDPOffset(Src2Reg - M65832::R0);
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);

    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(Src1DP);
    BuildMI(MBB, MI, DL, get(M65832::CLC));
    BuildMI(MBB, MI, DL, get(M65832::ADC_DP), M65832::A)
        .addReg(M65832::A)
        .addImm(Src2DP);
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  case M65832::SUB_GPR: {
    // SUB: dst = src1 - src2 using A (destructive)
    Register DstReg = MI.getOperand(0).getReg();
    Register Src1Reg = MI.getOperand(1).getReg();
    Register Src2Reg = MI.getOperand(2).getReg();
    unsigned Src1DP = getDPOffset(Src1Reg - M65832::R0);
    unsigned Src2DP = getDPOffset(Src2Reg - M65832::R0);
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);

    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(Src1DP);
    BuildMI(MBB, MI, DL, get(M65832::SEC));
    BuildMI(MBB, MI, DL, get(M65832::SBC_DP), M65832::A)
        .addReg(M65832::A)
        .addImm(Src2DP);
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  case M65832::AND_GPR: {
    // AND: LDA src1; AND src2; STA dst
    Register DstReg = MI.getOperand(0).getReg();
    Register Src1Reg = MI.getOperand(1).getReg();
    Register Src2Reg = MI.getOperand(2).getReg();
    unsigned Src1DP = getDPOffset(Src1Reg - M65832::R0);
    unsigned Src2DP = getDPOffset(Src2Reg - M65832::R0);
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);

    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(Src1DP);
    BuildMI(MBB, MI, DL, get(M65832::AND_DP), M65832::A)
        .addReg(M65832::A)
        .addImm(Src2DP);
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  case M65832::ORA_GPR: {
    // OR: LDA src1; ORA src2; STA dst
    Register DstReg = MI.getOperand(0).getReg();
    Register Src1Reg = MI.getOperand(1).getReg();
    Register Src2Reg = MI.getOperand(2).getReg();
    unsigned Src1DP = getDPOffset(Src1Reg - M65832::R0);
    unsigned Src2DP = getDPOffset(Src2Reg - M65832::R0);
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);

    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(Src1DP);
    BuildMI(MBB, MI, DL, get(M65832::ORA_DP), M65832::A)
        .addReg(M65832::A)
        .addImm(Src2DP);
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  case M65832::EOR_GPR: {
    // XOR: LDA src1; EOR src2; STA dst
    Register DstReg = MI.getOperand(0).getReg();
    Register Src1Reg = MI.getOperand(1).getReg();
    Register Src2Reg = MI.getOperand(2).getReg();
    unsigned Src1DP = getDPOffset(Src1Reg - M65832::R0);
    unsigned Src2DP = getDPOffset(Src2Reg - M65832::R0);
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);

    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(Src1DP);
    BuildMI(MBB, MI, DL, get(M65832::EOR_DP), M65832::A)
        .addReg(M65832::A)
        .addImm(Src2DP);
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  case M65832::ADDI_GPR: {
    // ADD immediate: dst = src + imm using A (destructive)
    Register DstReg = MI.getOperand(0).getReg();
    Register SrcReg = MI.getOperand(1).getReg();
    int64_t Imm = MI.getOperand(2).getImm();
    unsigned SrcDP = getDPOffset(SrcReg - M65832::R0);
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);

    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(SrcDP);
    BuildMI(MBB, MI, DL, get(M65832::CLC));
    BuildMI(MBB, MI, DL, get(M65832::ADC_IMM), M65832::A)
        .addReg(M65832::A)
        .addImm(Imm);
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  case M65832::SUBI_GPR: {
    // SUB immediate: dst = src - imm using A (destructive)
    Register DstReg = MI.getOperand(0).getReg();
    Register SrcReg = MI.getOperand(1).getReg();
    int64_t Imm = MI.getOperand(2).getImm();
    unsigned SrcDP = getDPOffset(SrcReg - M65832::R0);
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);

    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(SrcDP);
    BuildMI(MBB, MI, DL, get(M65832::SEC));
    BuildMI(MBB, MI, DL, get(M65832::SBC_IMM), M65832::A)
        .addReg(M65832::A)
        .addImm(Imm);
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  case M65832::ANDI_GPR: {
    // AND immediate: LDA src; AND #imm; STA dst
    Register DstReg = MI.getOperand(0).getReg();
    Register SrcReg = MI.getOperand(1).getReg();
    int64_t Imm = MI.getOperand(2).getImm();
    unsigned SrcDP = getDPOffset(SrcReg - M65832::R0);
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);

    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(SrcDP);
    BuildMI(MBB, MI, DL, get(M65832::AND_IMM), M65832::A)
        .addReg(M65832::A)
        .addImm(Imm);
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  case M65832::ORI_GPR: {
    // OR immediate: LDA src; ORA #imm; STA dst
    Register DstReg = MI.getOperand(0).getReg();
    Register SrcReg = MI.getOperand(1).getReg();
    int64_t Imm = MI.getOperand(2).getImm();
    unsigned SrcDP = getDPOffset(SrcReg - M65832::R0);
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);

    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(SrcDP);
    BuildMI(MBB, MI, DL, get(M65832::ORA_IMM), M65832::A)
        .addReg(M65832::A)
        .addImm(Imm);
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  case M65832::XORI_GPR: {
    // XOR immediate: LDA src; EOR #imm; STA dst
    Register DstReg = MI.getOperand(0).getReg();
    Register SrcReg = MI.getOperand(1).getReg();
    int64_t Imm = MI.getOperand(2).getImm();
    unsigned SrcDP = getDPOffset(SrcReg - M65832::R0);
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);

    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(SrcDP);
    BuildMI(MBB, MI, DL, get(M65832::EOR_IMM), M65832::A)
        .addReg(M65832::A)
        .addImm(Imm);
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  case M65832::LOAD32: {
    // Load from memory address: LDA (base+offset); STA dst
    Register DstReg = MI.getOperand(0).getReg();
    
    // Check operand types - base can be a register or frame index
    if (!MI.getOperand(1).isReg()) {
      // Frame index or other - should have been eliminated, treat as error
      llvm_unreachable("LOAD32 operand 1 should be a register after frame index elimination");
    }
    
    Register BaseReg = MI.getOperand(1).getReg();
    int64_t Offset = 0;
    if (MI.getNumOperands() > 2 && MI.getOperand(2).isImm()) {
      Offset = MI.getOperand(2).getImm();
    }
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);

    // For frame index, BaseReg will be FP (R29) or SP
    // Use indexed indirect addressing via Y
    if (BaseReg == M65832::B) {
      // Use B+offset addressing
      BuildMI(MBB, MI, DL, get(M65832::LDA_ABS), M65832::A)
          .addImm(Offset);
    } else if (BaseReg == M65832::R29 || BaseReg == M65832::SP) {
      // Load base pointer into a temp register and use indirect addressing
      unsigned BaseDP = getDPOffset(29); // R29 = FP
      if (BaseReg == M65832::SP) {
        // TSX; TXA; store to temp; then use temp as base
        BuildMI(MBB, MI, DL, get(M65832::TSX), M65832::X);
        BuildMI(MBB, MI, DL, get(M65832::TXA), M65832::A).addReg(M65832::X);
        // For simplicity, compute effective address and load
        if (Offset != 0) {
          BuildMI(MBB, MI, DL, get(M65832::CLC));
          BuildMI(MBB, MI, DL, get(M65832::ADC_IMM), M65832::A)
              .addReg(M65832::A)
              .addImm(Offset);
        }
        BuildMI(MBB, MI, DL, get(M65832::TAX), M65832::X).addReg(M65832::A);
        // LDA 0,X
        BuildMI(MBB, MI, DL, get(M65832::LDA_ABS_X), M65832::A)
            .addImm(0)
            .addReg(M65832::X);
      } else {
        // Use frame pointer - set Y to offset, then use indirect indexed
        BuildMI(MBB, MI, DL, get(M65832::LDY_IMM), M65832::Y).addImm(Offset);
        BuildMI(MBB, MI, DL, get(M65832::LDA_IND_Y), M65832::A)
            .addImm(BaseDP);
      }
    } else {
      // Regular GPR base
      unsigned BaseDP = getDPOffset(BaseReg - M65832::R0);
      BuildMI(MBB, MI, DL, get(M65832::LDY_IMM), M65832::Y).addImm(Offset);
      BuildMI(MBB, MI, DL, get(M65832::LDA_IND_Y), M65832::A)
          .addImm(BaseDP);
    }
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  case M65832::LOAD32_GLOBAL: {
    // Load from global address: LDA global; STA dst
    Register DstReg = MI.getOperand(0).getReg();
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);

    BuildMI(MBB, MI, DL, get(M65832::LDA_ABS), M65832::A)
        .add(MI.getOperand(1)); // Copy the global address operand
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  case M65832::STORE32: {
    // Store to memory address: LDA src; STA (base+offset)
    Register SrcReg = MI.getOperand(0).getReg();
    Register BaseReg = MI.getOperand(1).getReg();
    int64_t Offset = 0;
    if (MI.getNumOperands() > 2 && MI.getOperand(2).isImm()) {
      Offset = MI.getOperand(2).getImm();
    }
    unsigned SrcDP = getDPOffset(SrcReg - M65832::R0);

    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(SrcDP);

    if (BaseReg == M65832::B) {
      BuildMI(MBB, MI, DL, get(M65832::STA_ABS))
          .addReg(M65832::A, RegState::Kill)
          .addImm(Offset);
    } else if (BaseReg == M65832::R29 || BaseReg == M65832::SP) {
      unsigned BaseDP = getDPOffset(29);
      if (BaseReg == M65832::SP) {
        // Save A, compute address, store
        // Note: PHA lowers SP by 4 (32-bit push), so we must add 4 to offset
        // to compensate: effective_addr = (SP - 4) + (Offset + 4) = SP + Offset
        BuildMI(MBB, MI, DL, get(M65832::PHA)).addReg(M65832::A);
        BuildMI(MBB, MI, DL, get(M65832::TSX), M65832::X);
        BuildMI(MBB, MI, DL, get(M65832::TXA), M65832::A).addReg(M65832::X);
        // Always add (Offset + 4) to compensate for PHA
        int64_t AdjustedOffset = Offset + 4;
        BuildMI(MBB, MI, DL, get(M65832::CLC));
        BuildMI(MBB, MI, DL, get(M65832::ADC_IMM), M65832::A)
            .addReg(M65832::A)
            .addImm(AdjustedOffset);
        BuildMI(MBB, MI, DL, get(M65832::TAX), M65832::X).addReg(M65832::A);
        BuildMI(MBB, MI, DL, get(M65832::PLA), M65832::A);
        BuildMI(MBB, MI, DL, get(M65832::STA_ABS_X))
            .addReg(M65832::A, RegState::Kill)
            .addImm(0)
            .addReg(M65832::X);
      } else {
        // Use frame pointer
        BuildMI(MBB, MI, DL, get(M65832::LDY_IMM), M65832::Y).addImm(Offset);
        BuildMI(MBB, MI, DL, get(M65832::STA_IND_Y))
            .addReg(M65832::A, RegState::Kill)
            .addImm(BaseDP);
      }
    } else {
      unsigned BaseDP = getDPOffset(BaseReg - M65832::R0);
      BuildMI(MBB, MI, DL, get(M65832::LDY_IMM), M65832::Y).addImm(Offset);
      BuildMI(MBB, MI, DL, get(M65832::STA_IND_Y))
          .addReg(M65832::A, RegState::Kill)
          .addImm(BaseDP);
    }
    break;
  }

  case M65832::STORE32_GLOBAL: {
    // Store to global address: LDA src; STA global
    Register SrcReg = MI.getOperand(0).getReg();
    unsigned SrcDP = getDPOffset(SrcReg - M65832::R0);

    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(SrcDP);
    BuildMI(MBB, MI, DL, get(M65832::STA_ABS))
        .addReg(M65832::A, RegState::Kill)
        .add(MI.getOperand(1)); // Copy the global address operand
    break;
  }

  case M65832::LOAD8:
  case M65832::LOAD8_GLOBAL: {
    // Load byte from memory, zero-extended to 32-bit
    // Uses Extended ALU LD.B instruction which explicitly encodes byte size
    Register DstReg = MI.getOperand(0).getReg();

    if (MI.getOpcode() == M65832::LOAD8_GLOBAL) {
      // Load byte from global address using Extended ALU LD.B
      BuildMI(MBB, MI, DL, get(M65832::LDB_ABS), DstReg)
          .add(MI.getOperand(1));
    } else {
      Register BaseReg = MI.getOperand(1).getReg();
      int64_t Offset = MI.getNumOperands() > 2 ? MI.getOperand(2).getImm() : 0;
      if (BaseReg == M65832::B) {
        // B-relative byte load using Extended ALU LD.B abs
        BuildMI(MBB, MI, DL, get(M65832::LDB_ABS), DstReg)
            .addImm(Offset);
      } else if (BaseReg == M65832::R29 || BaseReg == M65832::SP) {
        // Stack/frame-based: compute address, use indirect Y load
        // Store base address in a temp register, set Y to offset
        Register TempReg = M65832::R16;  // Use a temp register
        if (BaseReg == M65832::SP) {
          BuildMI(MBB, MI, DL, get(M65832::TSX), M65832::X);
          BuildMI(MBB, MI, DL, get(M65832::STX_DP))
              .addReg(M65832::X)
              .addImm(getDPOffset(TempReg - M65832::R0));
        } else {
          unsigned FrameDP = getDPOffset(29); // R29 = FP
          BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(FrameDP);
          BuildMI(MBB, MI, DL, get(M65832::STA_DP))
              .addReg(M65832::A, RegState::Kill)
              .addImm(getDPOffset(TempReg - M65832::R0));
        }
        BuildMI(MBB, MI, DL, get(M65832::LDY_IMM), M65832::Y).addImm(Offset);
        // Use Extended ALU LD.B with indirect Y addressing
        BuildMI(MBB, MI, DL, get(M65832::LDB_IND_Y), DstReg)
            .addReg(TempReg);
      } else {
        // Register indirect with Y offset - use Extended ALU LD.B (dp),Y
        BuildMI(MBB, MI, DL, get(M65832::LDY_IMM), M65832::Y).addImm(Offset);
        BuildMI(MBB, MI, DL, get(M65832::LDB_IND_Y), DstReg)
            .addReg(BaseReg);
      }
    }
    break;
  }

  case M65832::LOAD16:
  case M65832::LOAD16_GLOBAL: {
    // Load 16-bit from memory, zero-extended to 32-bit
    // Uses Extended ALU LD.W instruction which explicitly encodes word size
    Register DstReg = MI.getOperand(0).getReg();

    if (MI.getOpcode() == M65832::LOAD16_GLOBAL) {
      // Load word from global address using Extended ALU LD.W
      BuildMI(MBB, MI, DL, get(M65832::LDW_ABS), DstReg)
          .add(MI.getOperand(1));
    } else {
      Register BaseReg = MI.getOperand(1).getReg();
      int64_t Offset = MI.getNumOperands() > 2 ? MI.getOperand(2).getImm() : 0;
      if (BaseReg == M65832::B) {
        // B-relative word load using Extended ALU LD.W abs
        BuildMI(MBB, MI, DL, get(M65832::LDW_ABS), DstReg)
            .addImm(Offset);
      } else if (BaseReg == M65832::R29 || BaseReg == M65832::SP) {
        // Stack/frame-based: compute address, use indirect Y load
        Register TempReg = M65832::R16;  // Use a temp register
        if (BaseReg == M65832::SP) {
          BuildMI(MBB, MI, DL, get(M65832::TSX), M65832::X);
          BuildMI(MBB, MI, DL, get(M65832::STX_DP))
              .addReg(M65832::X)
              .addImm(getDPOffset(TempReg - M65832::R0));
        } else {
          unsigned FrameDP = getDPOffset(29); // R29 = FP
          BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(FrameDP);
          BuildMI(MBB, MI, DL, get(M65832::STA_DP))
              .addReg(M65832::A, RegState::Kill)
              .addImm(getDPOffset(TempReg - M65832::R0));
        }
        BuildMI(MBB, MI, DL, get(M65832::LDY_IMM), M65832::Y).addImm(Offset);
        // Use Extended ALU LD.W with indirect Y addressing
        BuildMI(MBB, MI, DL, get(M65832::LDW_IND_Y), DstReg)
            .addReg(TempReg);
      } else {
        // Register indirect with Y offset - use Extended ALU LD.W (dp),Y
        BuildMI(MBB, MI, DL, get(M65832::LDY_IMM), M65832::Y).addImm(Offset);
        BuildMI(MBB, MI, DL, get(M65832::LDW_IND_Y), DstReg)
            .addReg(BaseReg);
      }
    }
    break;
  }

  case M65832::STORE8:
  case M65832::STORE8_GLOBAL: {
    // Truncating store - store only low 8 bits
    // Uses Extended ALU ST.B instruction which explicitly encodes byte size
    Register SrcReg = MI.getOperand(0).getReg();

    if (MI.getOpcode() == M65832::STORE8_GLOBAL) {
      // Store byte to global address using Extended ALU ST.B
      BuildMI(MBB, MI, DL, get(M65832::STB_ABS))
          .addReg(SrcReg)
          .add(MI.getOperand(1));
    } else {
      Register BaseReg = MI.getOperand(1).getReg();
      int64_t Offset = MI.getNumOperands() > 2 ? MI.getOperand(2).getImm() : 0;
      if (BaseReg == M65832::B) {
        // B-relative byte store using Extended ALU ST.B abs
        BuildMI(MBB, MI, DL, get(M65832::STB_ABS))
            .addReg(SrcReg)
            .addImm(Offset);
      } else if (BaseReg == M65832::R29 || BaseReg == M65832::SP) {
        // Stack/frame-based: compute address, use indirect Y store
        Register TempReg = M65832::R16;  // Use a temp register
        if (BaseReg == M65832::SP) {
          BuildMI(MBB, MI, DL, get(M65832::TSX), M65832::X);
          BuildMI(MBB, MI, DL, get(M65832::STX_DP))
              .addReg(M65832::X)
              .addImm(getDPOffset(TempReg - M65832::R0));
        } else {
          unsigned FrameDP = getDPOffset(29); // R29 = FP
          BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(FrameDP);
          BuildMI(MBB, MI, DL, get(M65832::STA_DP))
              .addReg(M65832::A, RegState::Kill)
              .addImm(getDPOffset(TempReg - M65832::R0));
        }
        BuildMI(MBB, MI, DL, get(M65832::LDY_IMM), M65832::Y).addImm(Offset);
        // Use Extended ALU ST.B with indirect Y addressing
        BuildMI(MBB, MI, DL, get(M65832::STB_IND_Y))
            .addReg(SrcReg)
            .addReg(TempReg);
      } else {
        // Register indirect with Y offset - use Extended ALU ST.B (dp),Y
        BuildMI(MBB, MI, DL, get(M65832::LDY_IMM), M65832::Y).addImm(Offset);
        BuildMI(MBB, MI, DL, get(M65832::STB_IND_Y))
            .addReg(SrcReg)
            .addReg(BaseReg);
      }
    }
    break;
  }

  case M65832::STORE16:
  case M65832::STORE16_GLOBAL: {
    // Truncating store - store only low 16 bits
    // Uses Extended ALU ST.W instruction which explicitly encodes word size
    Register SrcReg = MI.getOperand(0).getReg();

    if (MI.getOpcode() == M65832::STORE16_GLOBAL) {
      // Store word to global address using Extended ALU ST.W
      BuildMI(MBB, MI, DL, get(M65832::STW_ABS))
          .addReg(SrcReg)
          .add(MI.getOperand(1));
    } else {
      Register BaseReg = MI.getOperand(1).getReg();
      int64_t Offset = MI.getNumOperands() > 2 ? MI.getOperand(2).getImm() : 0;
      if (BaseReg == M65832::B) {
        // B-relative word store using Extended ALU ST.W abs
        BuildMI(MBB, MI, DL, get(M65832::STW_ABS))
            .addReg(SrcReg)
            .addImm(Offset);
      } else if (BaseReg == M65832::R29 || BaseReg == M65832::SP) {
        // Stack/frame-based: compute address, use indirect Y store
        Register TempReg = M65832::R16;  // Use a temp register
        if (BaseReg == M65832::SP) {
          BuildMI(MBB, MI, DL, get(M65832::TSX), M65832::X);
          BuildMI(MBB, MI, DL, get(M65832::STX_DP))
              .addReg(M65832::X)
              .addImm(getDPOffset(TempReg - M65832::R0));
        } else {
          unsigned FrameDP = getDPOffset(29); // R29 = FP
          BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(FrameDP);
          BuildMI(MBB, MI, DL, get(M65832::STA_DP))
              .addReg(M65832::A, RegState::Kill)
              .addImm(getDPOffset(TempReg - M65832::R0));
        }
        BuildMI(MBB, MI, DL, get(M65832::LDY_IMM), M65832::Y).addImm(Offset);
        // Use Extended ALU ST.W with indirect Y addressing
        BuildMI(MBB, MI, DL, get(M65832::STW_IND_Y))
            .addReg(SrcReg)
            .addReg(TempReg);
      } else {
        // Register indirect with Y offset - use Extended ALU ST.W (dp),Y
        BuildMI(MBB, MI, DL, get(M65832::LDY_IMM), M65832::Y).addImm(Offset);
        BuildMI(MBB, MI, DL, get(M65832::STW_IND_Y))
            .addReg(SrcReg)
            .addReg(BaseReg);
      }
    }
    break;
  }

  case M65832::JSR_IND: {
    // Indirect call through register (function pointer)
    // Load the target address into A, then use JSR (dp) where dp holds the address
    Register TargetReg = MI.getOperand(0).getReg();
    unsigned TargetDP = getDPOffset(TargetReg - M65832::R0);
    
    // JSR indirect uses JSR (dp) - opcode $FC dp
    // The dp location contains the 32-bit target address
    BuildMI(MBB, MI, DL, get(M65832::JSR_DP_IND))
        .addImm(TargetDP);
    break;
  }

  // FPU Load/Store pseudo expansions
  // FPU supports: LDF Fn, dp | LDF Fn, abs | LDF Fn, (Rm)
  
  case M65832::LDF32_GLOBAL:
  case M65832::LDF64_GLOBAL: {
    // Load float from global address into FPU register
    // Note: Using 64-bit LDF_abs for both f32/f64 until assembler supports LDF.S
    Register DstReg = MI.getOperand(0).getReg();
    
    // TODO: Use LDF_S_ind for f32 when assembler properly supports LDF.S
    // For now, use LDF_abs which loads 64 bits (but we only use low 32 for f32)
    BuildMI(MBB, MI, DL, get(M65832::LDF_abs), DstReg)
        .add(MI.getOperand(1)); // The global address symbol
    break;
  }

  case M65832::LDF32:
  case M65832::LDF64: {
    // Load float from memory (base+offset) into FPU register
    // Use register-indirect mode: LDF Fn, (Rm) - but Rm must be R0-R15!
    Register DstReg = MI.getOperand(0).getReg();
    Register BaseReg = MI.getOperand(1).getReg();
    int64_t Offset = 0;
    if (MI.getNumOperands() > 2 && MI.getOperand(2).isImm()) {
      Offset = MI.getOperand(2).getImm();
    }
    
    // Use LDF.S for f32 (32-bit), LDF for f64 (64-bit)
    bool IsSingle = (MI.getOpcode() == M65832::LDF32);
    unsigned LoadOpc = IsSingle ? M65832::LDF_S_ind : M65832::LDF_ind;
    
    // Check if BaseReg is in R0-R15 range (required for LDF/STF indirect)
    bool BaseIsLowReg = (BaseReg >= M65832::R0 && BaseReg <= M65832::R15);
    
    if (Offset == 0 && BaseIsLowReg) {
      // Simple case: LDF Fn, (Rm) with R0-R15
      BuildMI(MBB, MI, DL, get(LoadOpc), DstReg)
          .addReg(BaseReg);
    } else if (BaseReg == M65832::B) {
      // B register: compute effective address B+Offset into R0
      // The frame base was saved to R30's DP slot during prologue (STA R30; SB R30)
      // Load frame base from R30 slot, add offset, store to R0 for indirect load
      BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A)
          .addImm(getDPOffset(30)); // R30 contains frame base
      if (Offset != 0) {
        BuildMI(MBB, MI, DL, get(M65832::CLC));
        BuildMI(MBB, MI, DL, get(M65832::ADC_IMM), M65832::A)
            .addReg(M65832::A)
            .addImm(Offset);
      }
      BuildMI(MBB, MI, DL, get(M65832::STA_DP))
          .addReg(M65832::A, RegState::Kill)
          .addImm(getDPOffset(0)); // R0
      BuildMI(MBB, MI, DL, get(LoadOpc), DstReg)
          .addReg(M65832::R0);
    } else {
      // Need to compute address and/or copy to low register
      // Load base address into A
      if (BaseReg == M65832::SP) {
        // Stack pointer: TSX; TXA
        BuildMI(MBB, MI, DL, get(M65832::TSX));
        BuildMI(MBB, MI, DL, get(M65832::TXA));
      } else if (BaseIsLowReg) {
        // R0-R15: LDA via direct page
        unsigned BaseDP = getDPOffset(BaseReg - M65832::R0);
        BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(BaseDP);
      } else {
        // High register (R16+): use extended direct page addressing
        unsigned BaseDP = getDPOffset(BaseReg - M65832::R0);
        BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(BaseDP);
      }
      
      // Add offset if non-zero
      if (Offset != 0) {
        BuildMI(MBB, MI, DL, get(M65832::CLC));
        BuildMI(MBB, MI, DL, get(M65832::ADC_IMM), M65832::A)
            .addReg(M65832::A)
            .addImm(Offset);
      }
      
      // Store computed address to R0 (temp)
      BuildMI(MBB, MI, DL, get(M65832::STA_DP))
          .addReg(M65832::A, RegState::Kill)
          .addImm(getDPOffset(0)); // R0
      // Now load from address in R0
      BuildMI(MBB, MI, DL, get(LoadOpc), DstReg)
          .addReg(M65832::R0);
    }
    break;
  }

  case M65832::STF32_GLOBAL:
  case M65832::STF64_GLOBAL: {
    // Store float from FPU register to global address
    // Note: Using 64-bit STF_abs for both f32/f64 until assembler supports STF.S
    Register SrcReg = MI.getOperand(0).getReg();
    
    // TODO: Use STF_S_ind for f32 when assembler properly supports STF.S
    BuildMI(MBB, MI, DL, get(M65832::STF_abs))
        .addReg(SrcReg)
        .add(MI.getOperand(1)); // The global address symbol
    break;
  }

  case M65832::STF32:
  case M65832::STF64: {
    // Store float from FPU register to memory (base+offset)
    // Use register-indirect mode: STF Fn, (Rm) - but Rm must be R0-R15!
    Register SrcReg = MI.getOperand(0).getReg();
    Register BaseReg = MI.getOperand(1).getReg();
    int64_t Offset = 0;
    if (MI.getNumOperands() > 2 && MI.getOperand(2).isImm()) {
      Offset = MI.getOperand(2).getImm();
    }
    
    // Use STF.S for f32 (32-bit), STF for f64 (64-bit)
    bool IsSingle = (MI.getOpcode() == M65832::STF32);
    unsigned StoreOpc = IsSingle ? M65832::STF_S_ind : M65832::STF_ind;
    
    // Check if BaseReg is in R0-R15 range (required for LDF/STF indirect)
    bool BaseIsLowReg = (BaseReg >= M65832::R0 && BaseReg <= M65832::R15);
    
    if (Offset == 0 && BaseIsLowReg) {
      // Simple case: STF Fn, (Rm) with R0-R15
      BuildMI(MBB, MI, DL, get(StoreOpc))
          .addReg(SrcReg)
          .addReg(BaseReg);
    } else if (BaseReg == M65832::B) {
      // B register: compute effective address B+Offset into R0
      // The frame base was saved to R30's DP slot during prologue (STA R30; SB R30)
      BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A)
          .addImm(getDPOffset(30)); // R30 contains frame base
      if (Offset != 0) {
        BuildMI(MBB, MI, DL, get(M65832::CLC));
        BuildMI(MBB, MI, DL, get(M65832::ADC_IMM), M65832::A)
            .addReg(M65832::A)
            .addImm(Offset);
      }
      BuildMI(MBB, MI, DL, get(M65832::STA_DP))
          .addReg(M65832::A, RegState::Kill)
          .addImm(getDPOffset(0)); // R0
      BuildMI(MBB, MI, DL, get(StoreOpc))
          .addReg(SrcReg)
          .addReg(M65832::R0);
    } else {
      // Need to compute address and/or copy to low register
      // Load base address into A
      if (BaseReg == M65832::SP) {
        // Stack pointer: TSX; TXA
        BuildMI(MBB, MI, DL, get(M65832::TSX));
        BuildMI(MBB, MI, DL, get(M65832::TXA));
      } else if (BaseIsLowReg) {
        // R0-R15: LDA via direct page
        unsigned BaseDP = getDPOffset(BaseReg - M65832::R0);
        BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(BaseDP);
      } else {
        // High register (R16+): use extended direct page addressing
        unsigned BaseDP = getDPOffset(BaseReg - M65832::R0);
        BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(BaseDP);
      }
      
      // Add offset if non-zero
      if (Offset != 0) {
        BuildMI(MBB, MI, DL, get(M65832::CLC));
        BuildMI(MBB, MI, DL, get(M65832::ADC_IMM), M65832::A)
            .addReg(M65832::A)
            .addImm(Offset);
      }
      
      // Store computed address to R0 (temp)
      BuildMI(MBB, MI, DL, get(M65832::STA_DP))
          .addReg(M65832::A, RegState::Kill)
          .addImm(getDPOffset(0)); // R0
      // Now store to address in R0
      BuildMI(MBB, MI, DL, get(StoreOpc))
          .addReg(SrcReg)
          .addReg(M65832::R0);
    }
    break;
  }

  // FPU Float-to-Integer conversion pseudos
  // F2I.S Fd puts result in A, then store to GPR
  case M65832::F2I_S: {
    Register DstReg = MI.getOperand(0).getReg();
    Register SrcFPR = MI.getOperand(1).getReg();
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);
    
    // F2I.S Fd - converts Fd to int, result in A
    BuildMI(MBB, MI, DL, get(M65832::F2I_S_real))
        .addReg(SrcFPR);
    // Store A to destination GPR
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  case M65832::F2I_D: {
    Register DstReg = MI.getOperand(0).getReg();
    Register SrcFPR = MI.getOperand(1).getReg();
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);
    
    BuildMI(MBB, MI, DL, get(M65832::F2I_D_real))
        .addReg(SrcFPR);
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  // FPU Integer-to-Float conversion pseudos
  // Load GPR to A, then I2F.S Fd reads from A
  case M65832::I2F_S: {
    Register DstFPR = MI.getOperand(0).getReg();
    Register SrcReg = MI.getOperand(1).getReg();
    unsigned SrcDP = getDPOffset(SrcReg - M65832::R0);
    
    // Load source GPR into A
    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(SrcDP);
    // I2F.S Fd - converts A to float in Fd
    BuildMI(MBB, MI, DL, get(M65832::I2F_S_real), DstFPR);
    break;
  }

  case M65832::I2F_D: {
    Register DstFPR = MI.getOperand(0).getReg();
    Register SrcReg = MI.getOperand(1).getReg();
    unsigned SrcDP = getDPOffset(SrcReg - M65832::R0);
    
    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(SrcDP);
    BuildMI(MBB, MI, DL, get(M65832::I2F_D_real), DstFPR);
    break;
  }

  case M65832::SHL_GPR: {
    // Shift left: LDA src; repeat amt times: ASL A; STA dst
    Register DstReg = MI.getOperand(0).getReg();
    Register SrcReg = MI.getOperand(1).getReg();
    int64_t Amt = MI.getOperand(2).getImm();
    unsigned SrcDP = getDPOffset(SrcReg - M65832::R0);
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);

    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(SrcDP);
    for (int64_t i = 0; i < Amt && i < 32; ++i) {
      BuildMI(MBB, MI, DL, get(M65832::ASL_A), M65832::A).addReg(M65832::A);
    }
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  case M65832::SRL_GPR: {
    // Logical shift right: LDA src; repeat amt times: LSR A; STA dst
    Register DstReg = MI.getOperand(0).getReg();
    Register SrcReg = MI.getOperand(1).getReg();
    int64_t Amt = MI.getOperand(2).getImm();
    unsigned SrcDP = getDPOffset(SrcReg - M65832::R0);
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);

    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(SrcDP);
    for (int64_t i = 0; i < Amt && i < 32; ++i) {
      BuildMI(MBB, MI, DL, get(M65832::LSR_A), M65832::A).addReg(M65832::A);
    }
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  case M65832::SRA_GPR: {
    // Arithmetic shift right: This is trickier - need to preserve sign bit
    // For now, use LSR and manually handle sign extension if needed
    // A proper implementation would check bit 31 before each shift
    Register DstReg = MI.getOperand(0).getReg();
    Register SrcReg = MI.getOperand(1).getReg();
    int64_t Amt = MI.getOperand(2).getImm();
    unsigned SrcDP = getDPOffset(SrcReg - M65832::R0);
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);

    // Simple implementation: 
    // - If value is negative (bit 31 set), shift in 1s
    // - Otherwise shift in 0s (same as LSR)
    // For now, just do LSR (works for positive numbers)
    // TODO: Proper ASR implementation
    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(SrcDP);
    for (int64_t i = 0; i < Amt && i < 32; ++i) {
      // Use CMP to check sign, then conditionally ROR with C set
      // For simplicity, just LSR for now
      BuildMI(MBB, MI, DL, get(M65832::LSR_A), M65832::A).addReg(M65832::A);
    }
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  case M65832::INC_GPR: {
    // Increment in place: INC $dp
    Register Reg = MI.getOperand(0).getReg();
    unsigned DP = getDPOffset(Reg - M65832::R0);
    BuildMI(MBB, MI, DL, get(M65832::INC_DP)).addImm(DP);
    break;
  }

  case M65832::DEC_GPR: {
    // Decrement in place: DEC $dp
    Register Reg = MI.getOperand(0).getReg();
    unsigned DP = getDPOffset(Reg - M65832::R0);
    BuildMI(MBB, MI, DL, get(M65832::DEC_DP)).addImm(DP);
    break;
  }

  case M65832::STZ_GPR: {
    // Store zero: STZ $dp
    Register Reg = MI.getOperand(0).getReg();
    unsigned DP = getDPOffset(Reg - M65832::R0);
    BuildMI(MBB, MI, DL, get(M65832::STZ_DP)).addImm(DP);
    break;
  }

  case M65832::NEG_GPR: {
    // Negate: SEC; LDA #0; SBC src; STA dst
    Register DstReg = MI.getOperand(0).getReg();
    Register SrcReg = MI.getOperand(1).getReg();
    unsigned SrcDP = getDPOffset(SrcReg - M65832::R0);
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);

    BuildMI(MBB, MI, DL, get(M65832::SEC));
    BuildMI(MBB, MI, DL, get(M65832::LDA_IMM), M65832::A).addImm(0);
    BuildMI(MBB, MI, DL, get(M65832::SBC_DP), M65832::A)
        .addReg(M65832::A)
        .addImm(SrcDP);
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }

  case M65832::ASL_GPR: {
    // Direct memory shift left: ASL $dp (no accumulator needed!)
    Register Reg = MI.getOperand(0).getReg();
    unsigned DP = getDPOffset(Reg - M65832::R0);
    BuildMI(MBB, MI, DL, get(M65832::ASL_DP)).addImm(DP);
    break;
  }

  case M65832::LSR_GPR: {
    // Direct memory shift right: LSR $dp (no accumulator needed!)
    Register Reg = MI.getOperand(0).getReg();
    unsigned DP = getDPOffset(Reg - M65832::R0);
    BuildMI(MBB, MI, DL, get(M65832::LSR_DP)).addImm(DP);
    break;
  }

  case M65832::CMP_GPR: {
    // Compare two GPRs directly: CMP.L lhs, rhs
    Register LhsReg = MI.getOperand(0).getReg();
    Register RhsReg = MI.getOperand(1).getReg();
    BuildMI(MBB, MI, DL, get(M65832::CMPR_DP))
        .addReg(LhsReg)
        .addReg(RhsReg);
    break;
  }

  case M65832::CMP_GPR_IMM: {
    // Compare GPR with immediate: CMP.L lhs, #imm
    Register LhsReg = MI.getOperand(0).getReg();
    int64_t Imm = MI.getOperand(1).getImm();
    BuildMI(MBB, MI, DL, get(M65832::CMPR_IMM))
        .addReg(LhsReg)
        .addImm(Imm);
    break;
  }

  case M65832::BR_CC_CMP_PSEUDO: {
    // Fused compare-and-branch: CMP lhs, rhs; Bcc target
    Register LhsReg = MI.getOperand(0).getReg();
    Register RhsReg = MI.getOperand(1).getReg();
    int64_t CC = MI.getOperand(2).getImm();
    MachineBasicBlock *Target = MI.getOperand(3).getMBB();
    MachineBasicBlock *NextMBB = MBB.getNextNode();

    BuildMI(MBB, MI, DL, get(M65832::CMPR_DP))
        .addReg(LhsReg)
        .addReg(RhsReg);

    unsigned BrOpc;
    bool Emitted = false;
    switch (CC) {
    case ISD::SETEQ:  BrOpc = M65832::BEQ; break;
    case ISD::SETNE:  BrOpc = M65832::BNE; break;
    case ISD::SETLT:  BrOpc = M65832::BMI; break;
    case ISD::SETGE:  BrOpc = M65832::BPL; break;
    case ISD::SETGT:
      if (NextMBB) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addMBB(NextMBB);
        BuildMI(MBB, MI, DL, get(M65832::BMI)).addMBB(NextMBB);
        BuildMI(MBB, MI, DL, get(M65832::BRA)).addMBB(Target);
        Emitted = true;
        break;
      }
      BrOpc = M65832::BPL;
      break;
    case ISD::SETLE:
      if (NextMBB) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addMBB(Target);
        BuildMI(MBB, MI, DL, get(M65832::BMI)).addMBB(Target);
        Emitted = true;
        break;
      }
      BrOpc = M65832::BMI;
      break;
    case ISD::SETULT: BrOpc = M65832::BCC; break;
    case ISD::SETUGE: BrOpc = M65832::BCS; break;
    case ISD::SETUGT:
      if (NextMBB) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addMBB(NextMBB);
        BuildMI(MBB, MI, DL, get(M65832::BCS)).addMBB(Target);
        Emitted = true;
        break;
      }
      BrOpc = M65832::BCS;
      break;
    case ISD::SETULE:
      if (NextMBB) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addMBB(Target);
        BuildMI(MBB, MI, DL, get(M65832::BCC)).addMBB(Target);
        Emitted = true;
        break;
      }
      BrOpc = M65832::BCC;
      break;
    default:
      BrOpc = M65832::BNE;
      break;
    }

    if (!Emitted) {
      BuildMI(MBB, MI, DL, get(BrOpc)).addMBB(Target);
    }
    break;
  }

  case M65832::BR_CC_CMP_IMM_PSEUDO: {
    // Fused compare-and-branch with immediate: CMP lhs, #imm; Bcc target
    Register LhsReg = MI.getOperand(0).getReg();
    int64_t Imm = MI.getOperand(1).getImm();
    int64_t CC = MI.getOperand(2).getImm();
    MachineBasicBlock *Target = MI.getOperand(3).getMBB();
    MachineBasicBlock *NextMBB = MBB.getNextNode();

    BuildMI(MBB, MI, DL, get(M65832::CMPR_IMM))
        .addReg(LhsReg)
        .addImm(Imm);

    unsigned BrOpc;
    bool Emitted = false;
    switch (CC) {
    case ISD::SETEQ:  BrOpc = M65832::BEQ; break;
    case ISD::SETNE:  BrOpc = M65832::BNE; break;
    case ISD::SETLT:  BrOpc = M65832::BMI; break;
    case ISD::SETGE:  BrOpc = M65832::BPL; break;
    case ISD::SETGT:
      if (NextMBB) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addMBB(NextMBB);
        BuildMI(MBB, MI, DL, get(M65832::BMI)).addMBB(NextMBB);
        BuildMI(MBB, MI, DL, get(M65832::BRA)).addMBB(Target);
        Emitted = true;
        break;
      }
      BrOpc = M65832::BPL;
      break;
    case ISD::SETLE:
      if (NextMBB) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addMBB(Target);
        BuildMI(MBB, MI, DL, get(M65832::BMI)).addMBB(Target);
        Emitted = true;
        break;
      }
      BrOpc = M65832::BMI;
      break;
    case ISD::SETULT: BrOpc = M65832::BCC; break;
    case ISD::SETUGE: BrOpc = M65832::BCS; break;
    case ISD::SETUGT:
      if (NextMBB) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addMBB(NextMBB);
        BuildMI(MBB, MI, DL, get(M65832::BCS)).addMBB(Target);
        Emitted = true;
        break;
      }
      BrOpc = M65832::BCS;
      break;
    case ISD::SETULE:
      if (NextMBB) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addMBB(Target);
        BuildMI(MBB, MI, DL, get(M65832::BCC)).addMBB(Target);
        Emitted = true;
        break;
      }
      BrOpc = M65832::BCC;
      break;
    default:
      BrOpc = M65832::BNE;
      break;
    }

    if (!Emitted) {
      BuildMI(MBB, MI, DL, get(BrOpc)).addMBB(Target);
    }
    break;
  }

  case M65832::BR_CC_PSEUDO: {
    // Conditional branch based on condition code
    // The compare has already been done (via CMPR_DP), flags are set
    int64_t CC = MI.getOperand(0).getImm();
    MachineBasicBlock *Target = MI.getOperand(1).getMBB();
    MachineBasicBlock *NextMBB = MBB.getNextNode();
    
    unsigned BrOpc;
    bool Emitted = false;
    switch (CC) {
    case ISD::SETEQ:  BrOpc = M65832::BEQ; break;
    case ISD::SETNE:  BrOpc = M65832::BNE; break;
    case ISD::SETLT:  BrOpc = M65832::BMI; break;  // Signed less than
    case ISD::SETGE:  BrOpc = M65832::BPL; break;  // Signed greater or equal
    case ISD::SETGT:  // Signed greater than - need BNE + BPL combo
      if (NextMBB) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addMBB(NextMBB);
        BuildMI(MBB, MI, DL, get(M65832::BMI)).addMBB(NextMBB);
        BuildMI(MBB, MI, DL, get(M65832::BRA)).addMBB(Target);
        Emitted = true;
        break;
      }
      BrOpc = M65832::BPL;
      break;
    case ISD::SETLE:
      if (NextMBB) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addMBB(Target);
        BuildMI(MBB, MI, DL, get(M65832::BMI)).addMBB(Target);
        Emitted = true;
        break;
      }
      BrOpc = M65832::BMI;
      break;
    case ISD::SETULT: BrOpc = M65832::BCC; break;  // Unsigned less than
    case ISD::SETUGE: BrOpc = M65832::BCS; break;  // Unsigned greater or equal
    case ISD::SETUGT:
      if (NextMBB) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addMBB(NextMBB);
        BuildMI(MBB, MI, DL, get(M65832::BCS)).addMBB(Target);
        Emitted = true;
        break;
      }
      BrOpc = M65832::BCS;
      break;
    case ISD::SETULE:
      if (NextMBB) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addMBB(Target);
        BuildMI(MBB, MI, DL, get(M65832::BCC)).addMBB(Target);
        Emitted = true;
        break;
      }
      BrOpc = M65832::BCC;
      break;
    default:
      BrOpc = M65832::BNE;
      break;
    }
    
    if (!Emitted) {
      BuildMI(MBB, MI, DL, get(BrOpc)).addMBB(Target);
    }
    break;
  }

  case M65832::CMP_BR_CC: {
    // Fused compare-and-branch: CMP lhs, rhs; Bcc target
    // This pseudo is marked as a terminator, so PHI elimination inserts
    // copies BEFORE it, ensuring they don't clobber flags.
    Register LHSReg = MI.getOperand(0).getReg();
    Register RHSReg = MI.getOperand(1).getReg();
    int64_t CC = MI.getOperand(2).getImm();
    MachineBasicBlock *Target = MI.getOperand(3).getMBB();
    MachineBasicBlock *NextMBB = MBB.getNextNode();
    
    // Emit compare - IMMEDIATELY followed by branch
    BuildMI(MBB, MI, DL, get(M65832::CMPR_DP))
        .addReg(LHSReg)
        .addReg(RHSReg);
    
    // Map condition code to branch opcode
    unsigned BrOpc;
    bool Emitted = false;
    switch (CC) {
    case ISD::SETEQ:  BrOpc = M65832::BEQ; break;
    case ISD::SETNE:  BrOpc = M65832::BNE; break;
    case ISD::SETLT:  BrOpc = M65832::BMI; break;
    case ISD::SETGE:  BrOpc = M65832::BPL; break;
    case ISD::SETGT:
      if (NextMBB) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addMBB(NextMBB);
        BuildMI(MBB, MI, DL, get(M65832::BMI)).addMBB(NextMBB);
        BuildMI(MBB, MI, DL, get(M65832::BRA)).addMBB(Target);
        Emitted = true;
        break;
      }
      BrOpc = M65832::BPL;
      break;
    case ISD::SETLE:
      if (NextMBB) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addMBB(Target);
        BuildMI(MBB, MI, DL, get(M65832::BMI)).addMBB(Target);
        Emitted = true;
        break;
      }
      BrOpc = M65832::BMI;
      break;
    case ISD::SETULT: BrOpc = M65832::BCC; break;
    case ISD::SETUGE: BrOpc = M65832::BCS; break;
    case ISD::SETUGT:
      if (NextMBB) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addMBB(NextMBB);
        BuildMI(MBB, MI, DL, get(M65832::BCS)).addMBB(Target);
        Emitted = true;
        break;
      }
      BrOpc = M65832::BCS;
      break;
    case ISD::SETULE:
      if (NextMBB) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addMBB(Target);
        BuildMI(MBB, MI, DL, get(M65832::BCC)).addMBB(Target);
        Emitted = true;
        break;
      }
      BrOpc = M65832::BCC;
      break;
    default:
      BrOpc = M65832::BNE;
      break;
    }

    if (!Emitted) {
      BuildMI(MBB, MI, DL, get(BrOpc)).addMBB(Target);
    }
    break;
  }

  case M65832::SELECT_CC_PSEUDO: {
    // Conditional select: dst = (lhs cc rhs) ? trueVal : falseVal
    // Use inline branch sequence - MBB splitting causes iterator issues in expandPostRAPseudo
    // Operands: dst, lhs, rhs, trueVal, falseVal, cc
    Register DstReg = MI.getOperand(0).getReg();
    Register LHSReg = MI.getOperand(1).getReg();
    Register RHSReg = MI.getOperand(2).getReg();
    Register TrueReg = MI.getOperand(3).getReg();
    Register FalseReg = MI.getOperand(4).getReg();
    int64_t CC = MI.getOperand(5).getImm();
    
    // First, emit the CMP instruction to set flags
    // This ensures each SELECT_CC has its own comparison, regardless of
    // any flag-clobbering instructions scheduled between multiple SELECTs
    BuildMI(MBB, MI, DL, get(M65832::CMPR_DP))
        .addReg(LHSReg)
        .addReg(RHSReg);
    
    // Handle register aliasing: when TrueReg == DstReg, we must NOT clobber it
    // by copying FalseReg first. Instead, invert the logic:
    // - Normal: copy false first, skip true copy if condition false
    // - Inverted (when TrueReg == DstReg): skip false copy if condition true
    bool InvertLogic = (TrueReg == DstReg && TrueReg != FalseReg);
    
    // If FalseReg == DstReg, normal logic works (copy to self is nop, then maybe copy true)
    // If both alias DstReg, result is just DstReg regardless of condition
    if (TrueReg == DstReg && FalseReg == DstReg) {
      // Both source registers are the same as dest - nothing to do
      break;
    }
    
    // For inverted logic, we swap true/false and invert the condition
    Register FirstCopyReg = InvertLogic ? TrueReg : FalseReg;
    Register SecondCopyReg = InvertLogic ? FalseReg : TrueReg;
    
    // Map from normal condition to inverted condition
    auto InvertCC = [](int64_t cc) -> int64_t {
      switch (cc) {
      case ISD::SETEQ: return ISD::SETNE;
      case ISD::SETNE: return ISD::SETEQ;
      case ISD::SETLT: return ISD::SETGE;
      case ISD::SETGE: return ISD::SETLT;
      case ISD::SETGT: return ISD::SETLE;
      case ISD::SETLE: return ISD::SETGT;
      case ISD::SETULT: return ISD::SETUGE;
      case ISD::SETUGE: return ISD::SETULT;
      case ISD::SETUGT: return ISD::SETULE;
      case ISD::SETULE: return ISD::SETUGT;
      case ISD::SETOEQ: return ISD::SETONE;
      case ISD::SETONE: return ISD::SETOEQ;
      case ISD::SETOLT: return ISD::SETOGE;
      case ISD::SETOGE: return ISD::SETOLT;
      case ISD::SETOGT: return ISD::SETOLE;
      case ISD::SETOLE: return ISD::SETOGT;
      case ISD::SETUNE: return ISD::SETOEQ;
      default: return cc;
      }
    };
    
    int64_t EffectiveCC = InvertLogic ? InvertCC(CC) : CC;
    
    // First, unconditionally copy first value to destination
    // (Skip if it's already in destination - when FirstCopyReg == DstReg)
    if (FirstCopyReg != DstReg) {
      BuildMI(MBB, MI, DL, get(M65832::MOVR_DP), DstReg).addReg(FirstCopyReg);
    }
    
    // Then conditionally copy second value based on condition code
    // We need to skip the second copy if:
    // - Normal mode: condition is FALSE (keep false value)
    // - Inverted mode: condition is TRUE (keep true value)
    // After inversion, we always skip if "effective condition" is FALSE
    
    // For most conditions: BranchIfFalse skip; MOVR second->dst; skip:
    // MOVR_DP is 5 bytes, branch is 3 bytes
    
    unsigned SkipBrOpc;
    bool NeedDualBranch = false;
    
    switch (EffectiveCC) {
    case ISD::SETEQ:
    case ISD::SETOEQ:  SkipBrOpc = M65832::BNE; break;  // skip if NE
    case ISD::SETNE:
    case ISD::SETONE:
    case ISD::SETUNE:  SkipBrOpc = M65832::BEQ; break;  // skip if EQ
    case ISD::SETLT:
    case ISD::SETOLT:  SkipBrOpc = M65832::BPL; break;  // skip if >= (N=0)
    case ISD::SETGE:
    case ISD::SETOGE:  SkipBrOpc = M65832::BMI; break;  // skip if < (N=1)
    case ISD::SETGT:
    case ISD::SETOGT:  
      // GT: true if N=0 AND Z=0. Skip second copy if Z=1 OR N=1
      NeedDualBranch = true;
      break;
    case ISD::SETLE:
    case ISD::SETOLE:
      // LE: true if N=1 OR Z=1. Skip second copy if N=0 AND Z=0 (i.e., GT)
      NeedDualBranch = true;
      break;
    case ISD::SETULT:  SkipBrOpc = M65832::BCS; break;  // skip if >= (C=1)
    case ISD::SETUGE:  SkipBrOpc = M65832::BCC; break;  // skip if < (C=0)
    case ISD::SETUGT:
      // UGT: true if C=1 AND Z=0. Skip if Z=1 OR C=0
      NeedDualBranch = true;
      break;
    case ISD::SETULE:
      // ULE: true if C=0 OR Z=1. Skip if C=1 AND Z=0 (UGT)
      NeedDualBranch = true;
      break;
    default:           SkipBrOpc = M65832::BEQ; break;
    }
    
    if (NeedDualBranch) {
      // For GT/LE/UGT/ULE we need special handling
      // MOVR_DP is 5 bytes
      if (EffectiveCC == ISD::SETGT || EffectiveCC == ISD::SETOGT) {
        // GT: skip second copy if Z=1 OR N=1
        // BEQ at X: skip to X+11 (past 2 branches + MOVR), so *+11, offset = 8
        // BMI at X+3: skip to X+11, so *+8, offset = 5
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addImm(11);  // skip if equal (Z=1)
        BuildMI(MBB, MI, DL, get(M65832::BMI)).addImm(8);   // skip if less (N=1)
      } else if (EffectiveCC == ISD::SETLE || EffectiveCC == ISD::SETOLE) {
        // LE: copy if Z=1 OR N=1, skip only if GT (Z=0 AND N=0)
        // Structure: BEQ copy, BMI copy, BRA skip, MOVR, skip:
        // Note: addImm(N) is treated as *+N by assembler, encoded offset = N-3
        // BEQ at X: target MOVR at X+9, so *+9, offset = 6
        // BMI at X+3: target MOVR at X+9, so *+6, offset = 3  
        // BRA at X+6: target past MOVR at X+14, so *+8, offset = 5
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addImm(9);  // if equal, jump to copy
        BuildMI(MBB, MI, DL, get(M65832::BMI)).addImm(6);  // if less, jump to copy
        BuildMI(MBB, MI, DL, get(M65832::BRA)).addImm(8);  // else skip copy
      } else if (EffectiveCC == ISD::SETUGT) {
        // UGT: skip if Z=1 OR C=0
        // Note: addImm(N) is treated as *+N, encoded offset = N-3
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addImm(11);  // skip if equal
        BuildMI(MBB, MI, DL, get(M65832::BCC)).addImm(8);   // skip if carry clear
      } else if (EffectiveCC == ISD::SETULE) {
        // ULE: copy if Z=1 OR C=0, skip if UGT (C=1 AND Z=0)
        // Structure: BEQ copy, BCC copy, BRA skip, MOVR, skip:
        // Note: addImm(N) is treated as *+N, encoded offset = N-3
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addImm(9);  // if equal, jump to copy
        BuildMI(MBB, MI, DL, get(M65832::BCC)).addImm(6);  // if carry clear, jump to copy
        BuildMI(MBB, MI, DL, get(M65832::BRA)).addImm(8);  // else skip copy
      }
    } else {
      // Simple single branch: skip second copy if effective condition is false
      // MOVR_DP is 5 bytes, so target is *+8 (skip 3 byte branch + 5 byte MOVR)
      // Note: addImm(N) is treated as *+N, encoded offset = N-3
      BuildMI(MBB, MI, DL, get(SkipBrOpc)).addImm(8);
    }
    
    // Copy second value (only reached if effective condition is true)
    BuildMI(MBB, MI, DL, get(M65832::MOVR_DP), DstReg).addReg(SecondCopyReg);
    
    break;
  }

  case M65832::SELECT_CC_FP_PSEUDO: {
    // FP conditional select: dst = (cc) ? trueVal : falseVal
    // Flags are already set by FCMP (via glue), so no CMP needed
    // Operands: dst, trueVal, falseVal, cc
    Register DstReg = MI.getOperand(0).getReg();
    Register TrueReg = MI.getOperand(1).getReg();
    Register FalseReg = MI.getOperand(2).getReg();
    int64_t CC = MI.getOperand(3).getImm();
    
    // Handle register aliasing same as integer version
    bool InvertLogic = (TrueReg == DstReg && TrueReg != FalseReg);
    
    if (TrueReg == DstReg && FalseReg == DstReg) {
      break;  // Both aliases - nothing to do
    }
    
    Register FirstCopyReg = InvertLogic ? TrueReg : FalseReg;
    Register SecondCopyReg = InvertLogic ? FalseReg : TrueReg;
    
    auto InvertCC = [](int64_t cc) -> int64_t {
      switch (cc) {
      case ISD::SETEQ: return ISD::SETNE;
      case ISD::SETNE: return ISD::SETEQ;
      case ISD::SETLT: return ISD::SETGE;
      case ISD::SETGE: return ISD::SETLT;
      case ISD::SETGT: return ISD::SETLE;
      case ISD::SETLE: return ISD::SETGT;
      case ISD::SETULT: return ISD::SETUGE;
      case ISD::SETUGE: return ISD::SETULT;
      case ISD::SETUGT: return ISD::SETULE;
      case ISD::SETULE: return ISD::SETUGT;
      case ISD::SETOEQ: return ISD::SETONE;
      case ISD::SETONE: return ISD::SETOEQ;
      case ISD::SETOLT: return ISD::SETOGE;
      case ISD::SETOGE: return ISD::SETOLT;
      case ISD::SETOGT: return ISD::SETOLE;
      case ISD::SETOLE: return ISD::SETOGT;
      case ISD::SETUNE: return ISD::SETOEQ;
      default: return cc;
      }
    };
    
    int64_t EffectiveCC = InvertLogic ? InvertCC(CC) : CC;
    
    if (FirstCopyReg != DstReg) {
      BuildMI(MBB, MI, DL, get(M65832::MOVR_DP), DstReg).addReg(FirstCopyReg);
    }
    
    unsigned SkipBrOpc;
    bool NeedDualBranch = false;
    
    switch (EffectiveCC) {
    case ISD::SETEQ:
    case ISD::SETOEQ:  SkipBrOpc = M65832::BNE; break;
    case ISD::SETNE:
    case ISD::SETONE:
    case ISD::SETUNE:  SkipBrOpc = M65832::BEQ; break;
    case ISD::SETLT:
    case ISD::SETOLT:  SkipBrOpc = M65832::BPL; break;
    case ISD::SETGE:
    case ISD::SETOGE:  SkipBrOpc = M65832::BMI; break;
    case ISD::SETGT:
    case ISD::SETOGT:  NeedDualBranch = true; break;
    case ISD::SETLE:
    case ISD::SETOLE:  NeedDualBranch = true; break;
    case ISD::SETULT:  SkipBrOpc = M65832::BCS; break;
    case ISD::SETUGE:  SkipBrOpc = M65832::BCC; break;
    case ISD::SETUGT:  NeedDualBranch = true; break;
    case ISD::SETULE:  NeedDualBranch = true; break;
    default:           SkipBrOpc = M65832::BEQ; break;
    }
    
    if (NeedDualBranch) {
      if (EffectiveCC == ISD::SETGT || EffectiveCC == ISD::SETOGT) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addImm(11);
        BuildMI(MBB, MI, DL, get(M65832::BMI)).addImm(8);
      } else if (EffectiveCC == ISD::SETLE || EffectiveCC == ISD::SETOLE) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addImm(9);
        BuildMI(MBB, MI, DL, get(M65832::BMI)).addImm(6);
        BuildMI(MBB, MI, DL, get(M65832::BRA)).addImm(8);
      } else if (EffectiveCC == ISD::SETUGT) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addImm(11);
        BuildMI(MBB, MI, DL, get(M65832::BCC)).addImm(8);
      } else if (EffectiveCC == ISD::SETULE) {
        BuildMI(MBB, MI, DL, get(M65832::BEQ)).addImm(9);
        BuildMI(MBB, MI, DL, get(M65832::BCC)).addImm(6);
        BuildMI(MBB, MI, DL, get(M65832::BRA)).addImm(8);
      }
    } else {
      BuildMI(MBB, MI, DL, get(SkipBrOpc)).addImm(8);
    }
    
    BuildMI(MBB, MI, DL, get(M65832::MOVR_DP), DstReg).addReg(SecondCopyReg);
    
    break;
  }

  // Multiply/Divide pseudo expansions
  // Hardware instructions work on A and a memory operand:
  // MUL dp: A = A * [dp], high word in T
  // DIV dp: A = A / [dp], remainder in T
  
  case M65832::MUL_GPR: {
    // dst = src1 * src2: LDA src1; MUL src2; STA dst
    // MUL_DP: (outs ACC:$dst, TREG:$hi), (ins ACC:$src1, DPOp:$src2)
    Register DstReg = MI.getOperand(0).getReg();
    Register Src1Reg = MI.getOperand(1).getReg();
    Register Src2Reg = MI.getOperand(2).getReg();
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);
    unsigned Src1DP = getDPOffset(Src1Reg - M65832::R0);
    unsigned Src2DP = getDPOffset(Src2Reg - M65832::R0);
    
    // Load first operand into A
    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(Src1DP);
    // MUL: A = A * [dp], T = high word
    // Outputs: A (result), T (high)
    // Inputs: A (implicit), dp
    BuildMI(MBB, MI, DL, get(M65832::MUL_DP))
        .addReg(M65832::A, RegState::Define)  // $dst
        .addReg(M65832::T, RegState::Define | RegState::Dead)  // $hi (unused)
        .addReg(M65832::A)  // $src1
        .addImm(Src2DP);    // $src2 (DP address)
    // Store result
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }
  
  case M65832::SDIV_GPR:
  case M65832::UDIV_GPR: {
    // dst = src1 / src2: LDA src1; DIV src2; STA dst
    // DIV_DP: (outs ACC:$dst, TREG:$rem), (ins ACC:$src1, DPOp:$src2)
    Register DstReg = MI.getOperand(0).getReg();
    Register Src1Reg = MI.getOperand(1).getReg();
    Register Src2Reg = MI.getOperand(2).getReg();
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);
    unsigned Src1DP = getDPOffset(Src1Reg - M65832::R0);
    unsigned Src2DP = getDPOffset(Src2Reg - M65832::R0);
    
    // Load dividend into A
    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(Src1DP);
    // DIV: A = A / [dp], T = remainder
    unsigned DivOpc = (MI.getOpcode() == M65832::SDIV_GPR) ? M65832::DIV_DP : M65832::DIVU_DP;
    BuildMI(MBB, MI, DL, get(DivOpc))
        .addReg(M65832::A, RegState::Define)  // quotient
        .addReg(M65832::T, RegState::Define | RegState::Dead)  // remainder (unused)
        .addReg(M65832::A)  // dividend
        .addImm(Src2DP);    // divisor (DP address)
    // Store quotient
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }
  
  case M65832::SREM_GPR:
  case M65832::UREM_GPR: {
    // dst = src1 % src2: LDA src1; DIV src2; result is in T; TTA; STA dst
    Register DstReg = MI.getOperand(0).getReg();
    Register Src1Reg = MI.getOperand(1).getReg();
    Register Src2Reg = MI.getOperand(2).getReg();
    unsigned DstDP = getDPOffset(DstReg - M65832::R0);
    unsigned Src1DP = getDPOffset(Src1Reg - M65832::R0);
    unsigned Src2DP = getDPOffset(Src2Reg - M65832::R0);
    
    // Load dividend into A
    BuildMI(MBB, MI, DL, get(M65832::LDA_DP), M65832::A).addImm(Src1DP);
    // DIV: A = A / [dp], T = remainder
    unsigned DivOpc = (MI.getOpcode() == M65832::SREM_GPR) ? M65832::DIV_DP : M65832::DIVU_DP;
    BuildMI(MBB, MI, DL, get(DivOpc))
        .addReg(M65832::A, RegState::Define | RegState::Dead)  // quotient (unused)
        .addReg(M65832::T, RegState::Define)  // remainder
        .addReg(M65832::A)  // dividend
        .addImm(Src2DP);    // divisor (DP address)
    // Transfer remainder from T to A: TTA
    BuildMI(MBB, MI, DL, get(M65832::TTA), M65832::A);
    // Store remainder
    BuildMI(MBB, MI, DL, get(M65832::STA_DP))
        .addReg(M65832::A, RegState::Kill)
        .addImm(DstDP);
    break;
  }
  }

  MI.eraseFromParent();
  return true;
}
