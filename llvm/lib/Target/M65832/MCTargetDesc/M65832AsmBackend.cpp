//===-- M65832AsmBackend.cpp - M65832 Assembler Backend -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "M65832MCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {
class M65832AsmBackend : public MCAsmBackend {
  uint8_t OSABI;

public:
  M65832AsmBackend(uint8_t OSABI)
      : MCAsmBackend(llvm::endianness::little), OSABI(OSABI) {}

  ~M65832AsmBackend() override = default;

  void applyFixup(const MCFragment &F, const MCFixup &Fixup,
                  const MCValue &Target, uint8_t *Data, uint64_t Value,
                  bool IsResolved) override;

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return createM65832ELFObjectWriter(OSABI);
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *STI) const override {
    // NOP opcode is $EA
    for (uint64_t i = 0; i < Count; ++i)
      OS << '\xEA';
    return true;
  }
};

void M65832AsmBackend::applyFixup(const MCFragment &F, const MCFixup &Fixup,
                                    const MCValue &Target, uint8_t *Data,
                                    uint64_t Value, bool IsResolved) {
  unsigned Offset = Fixup.getOffset();
  unsigned Kind = Fixup.getKind();

  switch (Kind) {
  default:
    llvm_unreachable("Unknown fixup kind");
  case FK_Data_1:
    Data[Offset] = Value & 0xFF;
    break;
  case FK_Data_2:
    Data[Offset + 0] = Value & 0xFF;
    Data[Offset + 1] = (Value >> 8) & 0xFF;
    break;
  case FK_Data_4:
    Data[Offset + 0] = Value & 0xFF;
    Data[Offset + 1] = (Value >> 8) & 0xFF;
    Data[Offset + 2] = (Value >> 16) & 0xFF;
    Data[Offset + 3] = (Value >> 24) & 0xFF;
    break;
  }
}

} // end anonymous namespace

MCAsmBackend *llvm::createM65832AsmBackend(const Target &T,
                                            const MCSubtargetInfo &STI,
                                            const MCRegisterInfo &MRI,
                                            const MCTargetOptions &Options) {
  const Triple &TT = STI.getTargetTriple();
  uint8_t OSABI = MCELFObjectTargetWriter::getOSABI(TT.getOS());
  return new M65832AsmBackend(OSABI);
}
