//===-- M65832SelectionDAGInfo.h - M65832 SelectionDAG Info -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M65832_M65832SELECTIONDAGINFO_H
#define LLVM_LIB_TARGET_M65832_M65832SELECTIONDAGINFO_H

#include "llvm/CodeGen/SelectionDAGTargetInfo.h"

namespace llvm {

class M65832SelectionDAGInfo : public SelectionDAGTargetInfo {
public:
  explicit M65832SelectionDAGInfo() = default;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_M65832_M65832SELECTIONDAGINFO_H
