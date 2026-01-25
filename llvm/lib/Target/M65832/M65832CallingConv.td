//===-- M65832CallingConv.td - Calling Conventions ---------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// M65832 Calling Convention:
//
//   Arguments:    R0-R7 (first 8), then stack
//   Return:       R0 (32-bit), R0:R1 (64-bit)
//   Caller-saved: R0-R15, R30, R32-R47, A, X, Y
//   Callee-saved: R16-R23, R48-R55, R29
//   Reserved:     R24-R28, R31, R56-R63, SP, D, B, VBR, T
//
//   Stack grows downward, 4-byte alignment minimum.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Return Value Calling Convention
//===----------------------------------------------------------------------===//

def RetCC_M65832 : CallingConv<[
  // Promote small integers to i32
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,
  
  // i32 returns in R0
  CCIfType<[i32], CCAssignToReg<[R0]>>,
  
  // i64 returns in R0 (low) and R1 (high)
  CCIfType<[i64], CCAssignToRegWithShadow<[R0], [R1]>>,
  
  // f32 returns in R0 (bit-cast)
  CCIfType<[f32], CCAssignToReg<[R0]>>,
  
  // f64 returns in R0:R1
  CCIfType<[f64], CCAssignToRegWithShadow<[R0], [R1]>>
]>;

//===----------------------------------------------------------------------===//
// Argument Calling Convention
//===----------------------------------------------------------------------===//

def CC_M65832 : CallingConv<[
  // Promote small integers to i32
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,
  
  // First 8 i32 arguments in R0-R7
  CCIfType<[i32], CCAssignToReg<[R0, R1, R2, R3, R4, R5, R6, R7]>>,
  
  // i64 uses register pairs (even-aligned: R0:R1, R2:R3, R4:R5, R6:R7)
  CCIfType<[i64], CCAssignToRegWithShadow<[R0, R2, R4, R6], [R1, R3, R5, R7]>>,
  
  // f32 treated like i32
  CCIfType<[f32], CCAssignToReg<[R0, R1, R2, R3, R4, R5, R6, R7]>>,
  
  // f64 treated like i64
  CCIfType<[f64], CCAssignToRegWithShadow<[R0, R2, R4, R6], [R1, R3, R5, R7]>>,
  
  // Structs passed by value
  CCIfByVal<CCPassByVal<4, 4>>,
  
  // Remaining i32/f32 on stack, 4-byte slots, 4-byte aligned
  CCIfType<[i32, f32], CCAssignToStack<4, 4>>,
  
  // Remaining i64/f64 on stack, 8-byte slots, 4-byte aligned
  CCIfType<[i64, f64], CCAssignToStack<8, 4>>
]>;

//===----------------------------------------------------------------------===//
// Callee-Saved Registers
//===----------------------------------------------------------------------===//

// Standard callee-saved registers
def CSR_M65832 : CalleeSavedRegs<(add
  R16, R17, R18, R19, R20, R21, R22, R23,   // Primary callee-saved
  R48, R49, R50, R51, R52, R53, R54, R55,   // Extended callee-saved
  R29                                        // Frame pointer
)>;

// For interrupt handlers - save all volatile registers
def CSR_M65832_Interrupt : CalleeSavedRegs<(add
  // All GPRs used
  R0, R1, R2, R3, R4, R5, R6, R7,
  R8, R9, R10, R11, R12, R13, R14, R15,
  R16, R17, R18, R19, R20, R21, R22, R23,
  R30,
  R32, R33, R34, R35, R36, R37, R38, R39,
  R40, R41, R42, R43, R44, R45, R46, R47,
  R48, R49, R50, R51, R52, R53, R54, R55,
  R29,
  // Architectural registers
  A, X, Y
)>;

// No callee-saved (for special cases)
def CSR_M65832_NoRegs : CalleeSavedRegs<(add)>;
