//===-- M65832MCAsmInfo.cpp - M65832 Asm Properties -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of the M65832MCAsmInfo properties.
//
//===----------------------------------------------------------------------===//

#include "M65832MCAsmInfo.h"

using namespace llvm;

M65832MCAsmInfo::M65832MCAsmInfo(const Triple &TT) {
  // Use traditional 6502-style assembly syntax
  CommentString = ";";
  
  // Data directives
  Data8bitsDirective = "\t.byte\t";
  Data16bitsDirective = "\t.word\t";
  Data32bitsDirective = "\t.long\t";
  Data64bitsDirective = "\t.quad\t";
  
  // Zero directive
  ZeroDirective = "\t.zero\t";
  
  // ASCII directive
  AsciiDirective = "\t.ascii\t";
  AscizDirective = "\t.asciz\t";
  
  // Section switching
  SeparatorString = "\n";
  
  // Local labels
  PrivateGlobalPrefix = ".L";
  PrivateLabelPrefix = ".L";
  
  // Alignment
  AlignmentIsInBytes = true;
  
  // DWARF
  SupportsDebugInformation = true;
  ExceptionsType = ExceptionHandling::None;  // Match Triple default
  DwarfRegNumForCFI = true;
  
  // Code pointer size
  CodePointerSize = 4;
  CalleeSaveStackSlotSize = 4;
  
  // Little endian
  IsLittleEndian = true;
}
