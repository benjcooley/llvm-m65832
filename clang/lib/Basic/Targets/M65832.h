//===--- M65832.h - Declare M65832 target feature support -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares M65832 TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_M65832_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_M65832_H

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/Support/Compiler.h"
#include "llvm/TargetParser/Triple.h"

namespace clang {
namespace targets {

// M65832 Target - 32-bit 6502 derivative
class LLVM_LIBRARY_VISIBILITY M65832TargetInfo : public TargetInfo {
  static const char *const GCCRegNames[];
  static const TargetInfo::GCCRegAlias GCCRegAliases[];

public:
  M65832TargetInfo(const llvm::Triple &Triple, const TargetOptions &)
      : TargetInfo(Triple) {
    // M65832 is a 32-bit architecture
    TLSSupported = false;
    
    // Integer types
    IntWidth = 32;
    IntAlign = 32;
    ShortWidth = 16;
    ShortAlign = 16;
    LongWidth = 32;
    LongAlign = 32;
    LongLongWidth = 64;
    LongLongAlign = 64;
    
    // Pointer is 32-bit
    PointerWidth = 32;
    PointerAlign = 32;
    
    // Floating point
    HalfWidth = 16;
    HalfAlign = 16;
    FloatWidth = 32;
    FloatAlign = 32;
    DoubleWidth = 64;
    DoubleAlign = 64;
    LongDoubleWidth = 64;
    LongDoubleAlign = 64;
    
    // Alignment
    SuitableAlign = 32;
    DefaultAlignForAttributeAligned = 32;
    
    // Type mappings
    SizeType = UnsignedInt;
    PtrDiffType = SignedInt;
    IntPtrType = SignedInt;
    IntMaxType = SignedLongLong;
    Int64Type = SignedLongLong;
    SigAtomicType = SignedInt;
    
    // M65832 is little-endian
    BigEndian = false;
    
    resetDataLayout();
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;

  llvm::SmallVector<Builtin::InfosShard> getTargetBuiltins() const override {
    return {};
  }

  bool allowsLargerPreferedTypeAlignment() const override { return false; }

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::CharPtrBuiltinVaList;
  }

  std::string_view getClobbers() const override { return ""; }

  bool hasFeature(StringRef Feature) const override {
    return Feature == "m65832";
  }

  ArrayRef<const char *> getGCCRegNames() const override;

  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override;

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &Info) const override {
    switch (*Name) {
    default:
      return false;
    case 'r': // General purpose register
    case 'a': // Accumulator
    case 'x': // X index register
    case 'y': // Y index register
    case 'f': // FPU register
      Info.setAllowsRegister();
      return true;
    case 'I': // 8-bit immediate
      Info.setRequiresImmediate(0, 0xff);
      return true;
    case 'J': // 16-bit immediate
      Info.setRequiresImmediate(0, 0xffff);
      return true;
    }
    return false;
  }

  std::pair<unsigned, unsigned> hardwareInterferenceSizes() const override {
    // Cache line size - use reasonable defaults
    return std::make_pair(32, 32);
  }
};

} // namespace targets
} // namespace clang

#endif // LLVM_CLANG_LIB_BASIC_TARGETS_M65832_H
