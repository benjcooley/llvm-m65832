//===-- M65832MCAsmInfo.h - M65832 Asm Info ----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the M65832MCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M65832_MCTARGETDESC_M65832MCASMINFO_H
#define LLVM_LIB_TARGET_M65832_MCTARGETDESC_M65832MCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class M65832MCAsmInfo : public MCAsmInfoELF {
public:
  explicit M65832MCAsmInfo(const Triple &TT);
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_M65832_MCTARGETDESC_M65832MCASMINFO_H
