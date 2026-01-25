//===-- M65832.td - Describe the M65832 Target Machine -----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This is the top level entry point for the M65832 target.
// M65832 is a 32-bit evolution of the 6502/65816 architecture with:
// - 32-bit A, X, Y registers
// - 64 x 32-bit register window (R0-R63) accessible via Direct Page
// - Extended instructions via $02 prefix
// - Little-endian byte order
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Target-independent interfaces
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// Subtarget Features
//===----------------------------------------------------------------------===//

def FeatureFPU
 : SubtargetFeature<"fpu", "HasFPU", "true",
                    "Enable hardware floating point">;

def FeatureHWMul
 : SubtargetFeature<"hwmul", "HasHWMul", "true",
                    "Enable hardware multiply/divide (MUL, DIV instructions)">;

def FeatureAtomics
 : SubtargetFeature<"atomics", "HasAtomics", "true",
                    "Enable atomic operations (CAS, LLI, SCI)">;

//===----------------------------------------------------------------------===//
// M65832 supported processors
//===----------------------------------------------------------------------===//

class Proc<string Name, list<SubtargetFeature> Features>
 : Processor<Name, NoItineraries, Features>;

def : Proc<"generic",    [FeatureHWMul, FeatureAtomics]>;
def : Proc<"m65832",     [FeatureHWMul, FeatureAtomics]>;
def : Proc<"m65832-fpu", [FeatureHWMul, FeatureAtomics, FeatureFPU]>;

//===----------------------------------------------------------------------===//
// Register File Description
//===----------------------------------------------------------------------===//

include "M65832RegisterInfo.td"

//===----------------------------------------------------------------------===//
// Calling Convention Description
//===----------------------------------------------------------------------===//

include "M65832CallingConv.td"

//===----------------------------------------------------------------------===//
// Instruction Descriptions
//===----------------------------------------------------------------------===//

include "M65832InstrInfo.td"

defm : RemapAllTargetPseudoPointerOperands<GPR>;

def M65832InstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// Assembly Printers
//===----------------------------------------------------------------------===//

def M65832AsmWriter : AsmWriter {
  string AsmWriterClassName = "InstPrinter";
  bit isMCAsmWriter = 1;
}

//===----------------------------------------------------------------------===//
// Assembly Parsers
//===----------------------------------------------------------------------===//

def M65832AsmParser : AsmParser {
  let AllowDuplicateRegisterNames = 1;
  let ShouldEmitMatchRegisterAltName = 1;
}

//===----------------------------------------------------------------------===//
// Target Declaration
//===----------------------------------------------------------------------===//

def M65832 : Target {
  let InstructionSet = M65832InstrInfo;
  let AssemblyWriters = [M65832AsmWriter];
  let AssemblyParsers = [M65832AsmParser];
}
