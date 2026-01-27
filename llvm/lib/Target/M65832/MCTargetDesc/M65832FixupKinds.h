//===-- M65832FixupKinds.h - M65832 Specific Fixup Entries ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M65832_MCTARGETDESC_M65832FIXUPKINDS_H
#define LLVM_LIB_TARGET_M65832_MCTARGETDESC_M65832FIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace M65832 {

// This table must be in the same order of
// MCFixupKindInfo Infos[M65832::NumTargetFixupKinds]
// in M65832AsmBackend.cpp.
enum Fixups {
  // A 8 bit absolute fixup.
  fixup_m65832_8 = FirstTargetFixupKind,
  // A 16 bit absolute fixup.
  fixup_m65832_16,
  // A 24 bit absolute fixup (for bank + address).
  fixup_m65832_24,
  // A 32 bit absolute fixup.
  fixup_m65832_32,
  // A 8 bit PC relative fixup (for short branches).
  fixup_m65832_pcrel_8,
  // A 16 bit PC relative fixup (for long branches).
  fixup_m65832_pcrel_16,

  // Marker
  LastTargetFixupKind,
  NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};

} // end namespace M65832
} // end namespace llvm

#endif
