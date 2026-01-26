//===-- M65832TargetMachine.cpp - Define TargetMachine for M65832 ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Implements the info about M65832 target spec.
//
//===----------------------------------------------------------------------===//

#include "M65832TargetMachine.h"
#include "M65832.h"
#include "M65832MachineFunctionInfo.h"
#include "M65832TargetObjectFile.h"
#include "TargetInfo/M65832TargetInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Transforms/Scalar.h"

using namespace llvm;

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeM65832Target() {
  // Register the target.
  RegisterTargetMachine<M65832TargetMachine> X(getTheM65832Target());
}

static Reloc::Model getEffectiveRelocModel(std::optional<Reloc::Model> RM) {
  return RM.value_or(Reloc::Static);
}

// M65832 data layout:
// e = little endian
// m:e = ELF mangling
// p:32:32 = 32-bit pointers, 32-bit aligned
// i8:8, i16:16, i32:32, i64:64 = natural alignment
// f32:32, f64:64 = floating point alignment
// n32 = native 32-bit integers
// S32 = 32-bit stack alignment
static const char *M65832DataLayout = "e-m:e-p:32:32-i8:8-i16:16-i32:32-i64:64-f32:32-f64:64-n32-S32";

M65832TargetMachine::M65832TargetMachine(const Target &T, const Triple &TT,
                                           StringRef CPU, StringRef FS,
                                           const TargetOptions &Options,
                                           std::optional<Reloc::Model> RM,
                                           std::optional<CodeModel::Model> CM,
                                           CodeGenOptLevel OL, bool JIT)
    : CodeGenTargetMachineImpl(T, M65832DataLayout, TT, CPU, FS, Options,
                               getEffectiveRelocModel(RM),
                               getEffectiveCodeModel(CM, CodeModel::Small), OL),
      TLOF(std::make_unique<M65832TargetObjectFile>()),
      Subtarget(TT, std::string(CPU), std::string(FS), *this) {
  initAsmInfo();
}

namespace {
/// M65832 Code Generator Pass Configuration Options.
class M65832PassConfig : public TargetPassConfig {
public:
  M65832PassConfig(M65832TargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  M65832TargetMachine &getM65832TargetMachine() const {
    return getTM<M65832TargetMachine>();
  }

  bool addInstSelector() override;
  void addPreEmitPass() override;
};
} // namespace

TargetPassConfig *M65832TargetMachine::createPassConfig(PassManagerBase &PM) {
  return new M65832PassConfig(*this, PM);
}

bool M65832PassConfig::addInstSelector() {
  addPass(createM65832ISelDag(getM65832TargetMachine(), getOptLevel()));
  return false;
}

void M65832PassConfig::addPreEmitPass() {
  // Add any M65832-specific passes before code emission
}

MachineFunctionInfo *M65832TargetMachine::createMachineFunctionInfo(
    BumpPtrAllocator &Allocator, const Function &F,
    const TargetSubtargetInfo *STI) const {
  return M65832MachineFunctionInfo::create<M65832MachineFunctionInfo>(
      Allocator, F, STI);
}
