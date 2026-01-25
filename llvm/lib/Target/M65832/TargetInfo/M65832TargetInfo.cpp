//===-- M65832TargetInfo.cpp - M65832 Target Implementation ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/M65832TargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

Target &llvm::getTheM65832Target() {
  static Target TheM65832Target;
  return TheM65832Target;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeM65832TargetInfo() {
  RegisterTarget<Triple::m65832> X(getTheM65832Target(), "m65832",
                                    "M65832 [experimental]", "M65832");
}
