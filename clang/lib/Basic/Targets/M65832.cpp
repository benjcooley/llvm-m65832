//===--- M65832.cpp - Implement M65832 target feature support -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements M65832 TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#include "M65832.h"
#include "clang/Basic/MacroBuilder.h"

using namespace clang;
using namespace clang::targets;

const char *const M65832TargetInfo::GCCRegNames[] = {
    // General purpose registers R0-R63
    "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
    "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
    "r32", "r33", "r34", "r35", "r36", "r37", "r38", "r39",
    "r40", "r41", "r42", "r43", "r44", "r45", "r46", "r47",
    "r48", "r49", "r50", "r51", "r52", "r53", "r54", "r55",
    "r56", "r57", "r58", "r59", "r60", "r61", "r62", "r63",
    // Architectural registers
    "a",    // Accumulator
    "x",    // Index X
    "y",    // Index Y
    "sp",   // Stack pointer
    "d",    // Direct page base
    "b",    // Absolute base
    "vbr",  // Virtual base register
    "t",    // Temp (MUL high/DIV remainder)
    "sr"    // Status register
};

ArrayRef<const char *> M65832TargetInfo::getGCCRegNames() const {
  return llvm::ArrayRef(GCCRegNames);
}

void M65832TargetInfo::getTargetDefines(const LangOptions &Opts,
                                        MacroBuilder &Builder) const {
  // Standard M65832 defines
  Builder.defineMacro("__m65832__");
  Builder.defineMacro("__M65832__");
  Builder.defineMacro("M65832");
  
  // Architecture properties
  Builder.defineMacro("__SIZEOF_POINTER__", "4");
  Builder.defineMacro("__SIZEOF_INT__", "4");
  Builder.defineMacro("__SIZEOF_LONG__", "4");
  Builder.defineMacro("__SIZEOF_LONG_LONG__", "8");
  
  // Little endian
  Builder.defineMacro("__LITTLE_ENDIAN__");
  Builder.defineMacro("__ORDER_LITTLE_ENDIAN__", "1234");
  Builder.defineMacro("__BYTE_ORDER__", "__ORDER_LITTLE_ENDIAN__");
}
