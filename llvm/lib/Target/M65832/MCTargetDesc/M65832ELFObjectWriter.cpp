//===-- M65832ELFObjectWriter.cpp - M65832 ELF Writer ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/M65832FixupKinds.h"
#include "MCTargetDesc/M65832MCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {
class M65832ELFObjectWriter : public MCELFObjectTargetWriter {
public:
  M65832ELFObjectWriter(uint8_t OSABI)
      : MCELFObjectTargetWriter(/*Is64Bit=*/false, OSABI,
                                ELF::EM_M65832,
                                /*HasRelocationAddend=*/true) {}

  unsigned getRelocType(const MCFixup &Fixup, const MCValue &Target,
                        bool IsPCRel) const override;
};
} // end anonymous namespace

unsigned M65832ELFObjectWriter::getRelocType(const MCFixup &Fixup,
                                             const MCValue &Target,
                                             bool IsPCRel) const {
  unsigned Kind = Fixup.getKind();
  
  if (IsPCRel) {
    switch (Kind) {
    default:
      llvm_unreachable("Unsupported PC-relative relocation type");
    case FK_Data_1:
    case M65832::fixup_m65832_pcrel_8:
      return ELF::R_M65832_PCREL_8;
    case FK_Data_2:
    case M65832::fixup_m65832_pcrel_16:
      return ELF::R_M65832_PCREL_16;
    }
  }

  switch (Kind) {
  default:
    llvm_unreachable("Unsupported relocation type");
  case FK_Data_1:
  case M65832::fixup_m65832_8:
    return ELF::R_M65832_8;
  case FK_Data_2:
  case M65832::fixup_m65832_16:
    return ELF::R_M65832_16;
  case M65832::fixup_m65832_24:
    return ELF::R_M65832_24;
  case FK_Data_4:
  case M65832::fixup_m65832_32:
    return ELF::R_M65832_32;
  }
}

std::unique_ptr<MCObjectTargetWriter>
llvm::createM65832ELFObjectWriter(uint8_t OSABI) {
  return std::make_unique<M65832ELFObjectWriter>(OSABI);
}
