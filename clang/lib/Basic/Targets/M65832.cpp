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
    // FPU registers F0-F15
    "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",
    "f8",  "f9",  "f10", "f11", "f12", "f13", "f14", "f15",
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

// Map uppercase register names and common aliases to canonical lowercase names
const TargetInfo::GCCRegAlias M65832TargetInfo::GCCRegAliases[] = {
    // Uppercase GPR aliases R0-R63
    {{"R0"}, "r0"},   {{"R1"}, "r1"},   {{"R2"}, "r2"},   {{"R3"}, "r3"},
    {{"R4"}, "r4"},   {{"R5"}, "r5"},   {{"R6"}, "r6"},   {{"R7"}, "r7"},
    {{"R8"}, "r8"},   {{"R9"}, "r9"},   {{"R10"}, "r10"}, {{"R11"}, "r11"},
    {{"R12"}, "r12"}, {{"R13"}, "r13"}, {{"R14"}, "r14"}, {{"R15"}, "r15"},
    {{"R16"}, "r16"}, {{"R17"}, "r17"}, {{"R18"}, "r18"}, {{"R19"}, "r19"},
    {{"R20"}, "r20"}, {{"R21"}, "r21"}, {{"R22"}, "r22"}, {{"R23"}, "r23"},
    {{"R24"}, "r24"}, {{"R25"}, "r25"}, {{"R26"}, "r26"}, {{"R27"}, "r27"},
    {{"R28"}, "r28"}, {{"R29"}, "r29"}, {{"R30"}, "r30"}, {{"R31"}, "r31"},
    {{"R32"}, "r32"}, {{"R33"}, "r33"}, {{"R34"}, "r34"}, {{"R35"}, "r35"},
    {{"R36"}, "r36"}, {{"R37"}, "r37"}, {{"R38"}, "r38"}, {{"R39"}, "r39"},
    {{"R40"}, "r40"}, {{"R41"}, "r41"}, {{"R42"}, "r42"}, {{"R43"}, "r43"},
    {{"R44"}, "r44"}, {{"R45"}, "r45"}, {{"R46"}, "r46"}, {{"R47"}, "r47"},
    {{"R48"}, "r48"}, {{"R49"}, "r49"}, {{"R50"}, "r50"}, {{"R51"}, "r51"},
    {{"R52"}, "r52"}, {{"R53"}, "r53"}, {{"R54"}, "r54"}, {{"R55"}, "r55"},
    {{"R56"}, "r56"}, {{"R57"}, "r57"}, {{"R58"}, "r58"}, {{"R59"}, "r59"},
    {{"R60"}, "r60"}, {{"R61"}, "r61"}, {{"R62"}, "r62"}, {{"R63"}, "r63"},
    // Uppercase FPU aliases F0-F15
    {{"F0"}, "f0"},   {{"F1"}, "f1"},   {{"F2"}, "f2"},   {{"F3"}, "f3"},
    {{"F4"}, "f4"},   {{"F5"}, "f5"},   {{"F6"}, "f6"},   {{"F7"}, "f7"},
    {{"F8"}, "f8"},   {{"F9"}, "f9"},   {{"F10"}, "f10"}, {{"F11"}, "f11"},
    {{"F12"}, "f12"}, {{"F13"}, "f13"}, {{"F14"}, "f14"}, {{"F15"}, "f15"},
    // Uppercase architectural register aliases
    {{"A"}, "a"},
    {{"X"}, "x"},
    {{"Y"}, "y"},
    {{"SP"}, "sp"},
    {{"D"}, "d"},
    {{"B"}, "b"},
    {{"VBR"}, "vbr"},
    {{"T"}, "t"},
    {{"SR"}, "sr"},
    // Common named aliases
    {{"gp", "GP"}, "r28"},   // Global pointer
    {{"fp", "FP"}, "r29"},   // Frame pointer
    {{"lr", "LR"}, "r30"},   // Link register
};

ArrayRef<TargetInfo::GCCRegAlias> M65832TargetInfo::getGCCRegAliases() const {
  return llvm::ArrayRef(GCCRegAliases);
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
