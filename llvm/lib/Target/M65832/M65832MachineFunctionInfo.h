//===-- M65832MachineFunctionInfo.h - M65832 machine func info --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares M65832-specific per-machine-function information.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M65832_M65832MACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_M65832_M65832MACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {

class M65832MachineFunctionInfo : public MachineFunctionInfo {
  /// VarArgsFrameIndex - FrameIndex for start of varargs area.
  int VarArgsFrameIndex = 0;

  /// CalleeSavedFrameSize - Size of the callee-saved register portion of the
  /// stack frame in bytes.
  unsigned CalleeSavedFrameSize = 0;

  /// ReturnAddrIndex - FrameIndex for return slot.
  int ReturnAddrIndex = 0;

public:
  M65832MachineFunctionInfo() = default;
  
  explicit M65832MachineFunctionInfo(const Function &F,
                                      const TargetSubtargetInfo *STI) {}

  MachineFunctionInfo *
  clone(BumpPtrAllocator &Allocator, MachineFunction &DestMF,
        const DenseMap<MachineBasicBlock *, MachineBasicBlock *> &Src2DstMBB)
      const override {
    return DestMF.cloneInfo<M65832MachineFunctionInfo>(*this);
  }

  int getVarArgsFrameIndex() const { return VarArgsFrameIndex; }
  void setVarArgsFrameIndex(int Index) { VarArgsFrameIndex = Index; }

  unsigned getCalleeSavedFrameSize() const { return CalleeSavedFrameSize; }
  void setCalleeSavedFrameSize(unsigned Size) { CalleeSavedFrameSize = Size; }

  int getReturnAddrIndex() const { return ReturnAddrIndex; }
  void setReturnAddrIndex(int Index) { ReturnAddrIndex = Index; }
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_M65832_M65832MACHINEFUNCTIONINFO_H
