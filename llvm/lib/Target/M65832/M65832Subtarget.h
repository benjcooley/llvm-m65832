//===-- M65832Subtarget.h - Define Subtarget for M65832 ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the M65832 specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M65832_M65832SUBTARGET_H
#define LLVM_LIB_TARGET_M65832_M65832SUBTARGET_H

#include "M65832FrameLowering.h"
#include "M65832ISelLowering.h"
#include "M65832InstrInfo.h"
#include "M65832RegisterInfo.h"
#include "M65832SelectionDAGInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Target/TargetMachine.h"

#define GET_SUBTARGETINFO_HEADER
#include "M65832GenSubtargetInfo.inc"

namespace llvm {

class StringRef;

class M65832Subtarget : public M65832GenSubtargetInfo {
  Triple TargetTriple;
  
  // Subtarget features
  bool HasFPU = false;
  bool HasHWMul = true;
  bool HasAtomics = true;

  M65832InstrInfo InstrInfo;
  M65832FrameLowering FrameLowering;
  M65832TargetLowering TLInfo;
  M65832SelectionDAGInfo TSInfo;
  M65832RegisterInfo RegInfo;

public:
  M65832Subtarget(const Triple &TT, const std::string &CPU,
                   const std::string &FS, const TargetMachine &TM);

  /// ParseSubtargetFeatures - Parses features string setting specified
  /// subtarget options. Defined by tablegen in M65832GenSubtargetInfo.inc.
  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  const M65832InstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const M65832FrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const M65832TargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }
  const M65832SelectionDAGInfo *getSelectionDAGInfo() const override {
    return &TSInfo;
  }
  const M65832RegisterInfo *getRegisterInfo() const override {
    return &RegInfo;
  }

  bool hasFPU() const { return HasFPU; }
  bool hasHWMul() const { return HasHWMul; }
  bool hasAtomics() const { return HasAtomics; }
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_M65832_M65832SUBTARGET_H
