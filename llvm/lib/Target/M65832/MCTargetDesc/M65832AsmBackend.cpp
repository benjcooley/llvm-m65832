//===-- M65832AsmBackend.cpp - M65832 Assembler Backend -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/M65832FixupKinds.h"
#include "MCTargetDesc/M65832MCTargetDesc.h"
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

  MCFixupKindInfo getFixupKindInfo(MCFixupKind Kind) const override {
    // clang-format off
    const static MCFixupKindInfo Infos[M65832::NumTargetFixupKinds] = {
      // This table must be in the same order as enum in M65832FixupKinds.h.
      //
      // name                     offset bits flags
      {"fixup_m65832_8",          0,     8,   0},
      {"fixup_m65832_16",         0,     16,  0},
      {"fixup_m65832_24",         0,     24,  0},
      {"fixup_m65832_32",         0,     32,  0},
      {"fixup_m65832_pcrel_8",    0,     8,   0},
      {"fixup_m65832_pcrel_16",   0,     16,  0},
    };
    // clang-format on
    static_assert((std::size(Infos)) == M65832::NumTargetFixupKinds,
                  "Not all fixup kinds added to Infos array");

    if (Kind < FirstTargetFixupKind)
      return MCAsmBackend::getFixupKindInfo(Kind);

    assert(unsigned(Kind - FirstTargetFixupKind) < M65832::NumTargetFixupKinds &&
           "Invalid kind!");
    return Infos[Kind - FirstTargetFixupKind];
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
  // Call maybeAddReloc to emit relocations for unresolved symbols
  maybeAddReloc(F, Fixup, Target, Value, IsResolved);
  
  if (!Value)
    return; // Nothing to apply

  unsigned Offset = Fixup.getOffset();
  unsigned Kind = Fixup.getKind();
  unsigned NumBytes;

  switch (Kind) {
  default:
    llvm_unreachable("Unknown fixup kind");
  case FK_Data_1:
  case M65832::fixup_m65832_8:
  case M65832::fixup_m65832_pcrel_8:
    NumBytes = 1;
    break;
  case FK_Data_2:
  case M65832::fixup_m65832_16:
  case M65832::fixup_m65832_pcrel_16:
    NumBytes = 2;
    break;
  case M65832::fixup_m65832_24:
    NumBytes = 3;
    break;
  case FK_Data_4:
  case M65832::fixup_m65832_32:
    NumBytes = 4;
    break;
  }

  // Write the fixup value in little-endian format
  for (unsigned i = 0; i < NumBytes; ++i) {
    Data[Offset + i] = Value & 0xFF;
    Value >>= 8;
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
