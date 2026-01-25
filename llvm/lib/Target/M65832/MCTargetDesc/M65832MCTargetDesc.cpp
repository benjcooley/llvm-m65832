//===-- M65832MCTargetDesc.cpp - M65832 Target Descriptions ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides M65832 specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "M65832MCTargetDesc.h"
#include "M65832InstPrinter.h"
#include "M65832MCAsmInfo.h"
#include "TargetInfo/M65832TargetInfo.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#define GET_INSTRINFO_MC_HELPER_DEFS
#include "M65832GenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "M65832GenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "M65832GenRegisterInfo.inc"

static MCInstrInfo *createM65832MCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitM65832MCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createM65832MCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitM65832MCRegisterInfo(X, M65832::R30);
  return X;
}

static MCSubtargetInfo *createM65832MCSubtargetInfo(const Triple &TT,
                                                     StringRef CPU,
                                                     StringRef FS) {
  return createM65832MCSubtargetInfoImpl(TT, CPU, CPU, FS);
}

static MCInstPrinter *createM65832MCInstPrinter(const Triple &T,
                                                  unsigned SyntaxVariant,
                                                  const MCAsmInfo &MAI,
                                                  const MCInstrInfo &MII,
                                                  const MCRegisterInfo &MRI) {
  return new M65832InstPrinter(MAI, MII, MRI);
}

static MCAsmInfo *createM65832MCAsmInfo(const MCRegisterInfo &MRI,
                                         const Triple &TT,
                                         const MCTargetOptions &Options) {
  MCAsmInfo *MAI = new M65832MCAsmInfo(TT);

  // Initial state of the frame pointer is SP.
  MCCFIInstruction Inst = MCCFIInstruction::cfiDefCfa(nullptr, M65832::SP, 0);
  MAI->addInitialFrameState(Inst);

  return MAI;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeM65832TargetMC() {
  Target &T = getTheM65832Target();

  RegisterMCAsmInfoFn X(T, createM65832MCAsmInfo);
  TargetRegistry::RegisterMCInstrInfo(T, createM65832MCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(T, createM65832MCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(T, createM65832MCSubtargetInfo);
  TargetRegistry::RegisterMCInstPrinter(T, createM65832MCInstPrinter);
  TargetRegistry::RegisterMCCodeEmitter(T, createM65832MCCodeEmitter);
  TargetRegistry::RegisterMCAsmBackend(T, createM65832AsmBackend);
}
