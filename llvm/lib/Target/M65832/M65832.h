//===-- M65832.h - Top-level interface for M65832 ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the
// M65832 target library.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M65832_M65832_H
#define LLVM_LIB_TARGET_M65832_M65832_H

#include "MCTargetDesc/M65832MCTargetDesc.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class M65832TargetMachine;
class FunctionPass;

// Condition codes for branches
namespace M65832CC {
  enum CondCode {
    COND_EQ,   // Equal (Z=1)
    COND_NE,   // Not equal (Z=0)
    COND_CS,   // Carry set (C=1) - unsigned >=
    COND_CC,   // Carry clear (C=0) - unsigned <
    COND_MI,   // Minus (N=1)
    COND_PL,   // Plus (N=0)
    COND_VS,   // Overflow set (V=1)
    COND_VC,   // Overflow clear (V=0)
    COND_HI,   // Higher (unsigned >) - C=1 && Z=0
    COND_LS,   // Lower or same (unsigned <=) - C=0 || Z=1
    COND_GE,   // Greater or equal (signed) - N==V
    COND_LT,   // Less than (signed) - N!=V
    COND_GT,   // Greater than (signed) - Z=0 && N==V
    COND_LE,   // Less or equal (signed) - Z=1 || N!=V
    COND_INVALID
  };
} // namespace M65832CC

namespace M65832ISD {
  enum NodeType : unsigned {
    FIRST_NUMBER = ISD::BUILTIN_OP_END,
    
    // Return with flag
    RET_FLAG,
    
    // Subroutine call
    CALL,
    
    // Compare (sets flags) - integer
    CMP,
    
    // FP Compare (sets flags) - floating point  
    FCMP,
    
    // Branch on condition code
    BR_CC,

    // Fused compare-and-branch (single terminator)
    BR_CC_CMP,
    
    // Select on condition code (integer - includes LHS/RHS for CMP)
    SELECT_CC,
    
    // Select on condition code (integer comparison, any result type)
    SELECT_CC_MIXED,
    
    // Select on condition code (FP - uses glue from FCMP)
    SELECT_CC_FP,
    
    // Global/constant address wrapper
    WRAPPER,
    
    // Multiply returning high:low
    SMUL_LOHI,
    UMUL_LOHI,
    
    // Divide with remainder
    SDIVREM,
    UDIVREM,
  };
} // namespace M65832ISD

FunctionPass *createM65832ISelDag(M65832TargetMachine &TM,
                                   CodeGenOptLevel OptLevel);

} // namespace llvm

#endif // LLVM_LIB_TARGET_M65832_M65832_H
