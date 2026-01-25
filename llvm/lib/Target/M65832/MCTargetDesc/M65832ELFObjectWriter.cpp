//===-- M65832ELFObjectWriter.cpp - M65832 ELF Writer ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "M65832MCTargetDesc.h"
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
  // M65832 ELF machine type: 0x6583
  static constexpr unsigned EM_M65832 = 0x6583;

  M65832ELFObjectWriter(uint8_t OSABI)
      : MCELFObjectTargetWriter(/*Is64Bit=*/false, OSABI,
                                EM_M65832,
                                /*HasRelocationAddend=*/true) {}

  unsigned getRelocType(const MCFixup &Fixup, const MCValue &Target,
                        bool IsPCRel) const override;
};
} // end anonymous namespace

unsigned M65832ELFObjectWriter::getRelocType(const MCFixup &Fixup,
                                               const MCValue &Target,
                                               bool IsPCRel) const {
  unsigned Kind = Fixup.getKind();
  
  switch (Kind) {
  default:
    llvm_unreachable("Unsupported relocation type");
  case FK_Data_1:
    return ELF::R_X86_64_8;       // Placeholder
  case FK_Data_2:
    return ELF::R_X86_64_16;      // Placeholder
  case FK_Data_4:
    return ELF::R_X86_64_32;      // Placeholder
  }
}

std::unique_ptr<MCObjectTargetWriter>
llvm::createM65832ELFObjectWriter(uint8_t OSABI) {
  return std::make_unique<M65832ELFObjectWriter>(OSABI);
}
