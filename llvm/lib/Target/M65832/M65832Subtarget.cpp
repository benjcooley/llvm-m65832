//===-- M65832Subtarget.cpp - M65832 Subtarget Information ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the M65832 specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "M65832Subtarget.h"
#include "M65832.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "m65832-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "M65832GenSubtargetInfo.inc"

M65832Subtarget::M65832Subtarget(const Triple &TT, const std::string &CPU,
                                   const std::string &FS,
                                   const TargetMachine &TM)
    : M65832GenSubtargetInfo(TT, CPU, /*TuneCPU*/ CPU, FS),
      TargetTriple(TT),
      InstrInfo(*this),
      FrameLowering(*this),
      TLInfo(TM, *this),
      RegInfo(*this) {
  
  // Parse features string
  std::string CPUName = std::string(CPU);
  if (CPUName.empty())
    CPUName = "generic";
  
  ParseSubtargetFeatures(CPUName, CPUName, FS);
}
