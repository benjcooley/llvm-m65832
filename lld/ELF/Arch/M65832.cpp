//===- M65832.cpp ---------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The M65832 is a 32-bit extension of the 65C816 microprocessor architecture.
// It maintains compatibility with the 6502 instruction set while adding
// 32-bit operations, extended addressing modes, and floating-point support.
//
// Since it is a baremetal architecture, programs are typically linked against
// address 0 and the .text section is extracted using objcopy:
//
//   ld.lld -Ttext=0 -o foo foo.o
//   objcopy -O binary --only-section=.text foo output.bin
//
//===----------------------------------------------------------------------===//

#include "Symbols.h"
#include "Target.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Support/Endian.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::support::endian;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;


namespace {
class M65832 final : public TargetInfo {
public:
  M65832(Ctx &ctx);
  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const uint8_t *loc) const override;
  void relocate(uint8_t *loc, const Relocation &rel,
                uint64_t val) const override;
};
} // namespace

M65832::M65832(Ctx &ctx) : TargetInfo(ctx) {
  // NOP opcode is $EA (same as 6502)
  trapInstr = {0xEA, 0xEA, 0xEA, 0xEA};
  // Default page size for M65832 (64KB bank)
  defaultMaxPageSize = 0x10000;
  defaultImageBase = 0;
}

RelExpr M65832::getRelExpr(RelType type, const Symbol &s,
                           const uint8_t *loc) const {
  switch (type) {
  case R_M65832_PCREL_8:
  case R_M65832_PCREL_16:
    return R_PC;
  case R_M65832_8:
  case R_M65832_16:
  case R_M65832_24:
  case R_M65832_32:
    return R_ABS;
  default:
    Err(ctx) << getErrorLoc(ctx, loc) << "unknown relocation (" << type.v
             << ") against symbol " << &s;
    return R_NONE;
  }
}

void M65832::relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const {
  switch (rel.type) {
  case R_M65832_8:
    checkIntUInt(ctx, loc, val, 8, rel);
    *loc = val;
    break;
  case R_M65832_16:
    checkIntUInt(ctx, loc, val, 16, rel);
    write16le(loc, val);
    break;
  case R_M65832_24:
    checkIntUInt(ctx, loc, val, 24, rel);
    write16le(loc, val & 0xFFFF);
    loc[2] = (val >> 16) & 0xFF;
    break;
  case R_M65832_32:
    checkIntUInt(ctx, loc, val, 32, rel);
    write32le(loc, val);
    break;
  case R_M65832_PCREL_8: {
    int64_t offset = val;
    checkInt(ctx, loc, offset, 8, rel);
    *loc = offset;
    break;
  }
  case R_M65832_PCREL_16: {
    int64_t offset = val;
    checkInt(ctx, loc, offset, 16, rel);
    write16le(loc, offset);
    break;
  }
  default:
    Err(ctx) << getErrorLoc(ctx, loc) << "unrecognized relocation " << rel.type;
  }
}

void elf::setM65832TargetInfo(Ctx &ctx) { ctx.target.reset(new M65832(ctx)); }
