//===-- M65832MCTargetDesc.h - M65832 Target Descriptions -------*- C++ -*-===//
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

#ifndef LLVM_LIB_TARGET_M65832_MCTARGETDESC_M65832MCTARGETDESC_H
#define LLVM_LIB_TARGET_M65832_MCTARGETDESC_M65832MCTARGETDESC_H

#include "llvm/MC/MCTargetOptions.h"
#include <memory>

namespace llvm {
class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCObjectTargetWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class Target;

MCCodeEmitter *createM65832MCCodeEmitter(const MCInstrInfo &MCII,
                                          MCContext &Ctx);

MCAsmBackend *createM65832AsmBackend(const Target &T, const MCSubtargetInfo &STI,
                                      const MCRegisterInfo &MRI,
                                      const MCTargetOptions &Options);

std::unique_ptr<MCObjectTargetWriter> createM65832ELFObjectWriter(uint8_t OSABI);

} // namespace llvm

// Defines symbolic names for M65832 registers.
// This defines a mapping from register name to register number.
#define GET_REGINFO_ENUM
#include "M65832GenRegisterInfo.inc"

// Defines symbolic names for the M65832 instructions.
#define GET_INSTRINFO_ENUM
#define GET_INSTRINFO_MC_HELPER_DECLS
#include "M65832GenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "M65832GenSubtargetInfo.inc"

#endif // LLVM_LIB_TARGET_M65832_MCTARGETDESC_M65832MCTARGETDESC_H
