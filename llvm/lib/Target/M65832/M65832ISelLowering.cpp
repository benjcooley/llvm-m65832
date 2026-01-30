//===-- M65832ISelLowering.cpp - M65832 DAG Lowering Implementation -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the M65832TargetLowering class.
//
//===----------------------------------------------------------------------===//

#include "M65832ISelLowering.h"
#include "M65832.h"
#include "M65832MachineFunctionInfo.h"
#include "M65832Subtarget.h"
#include "M65832TargetMachine.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/MC/MCRegister.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/ErrorHandling.h"
#include <utility>

using namespace llvm;

#define DEBUG_TYPE "m65832-lower"

#include "M65832GenCallingConv.inc"

M65832TargetLowering::M65832TargetLowering(const TargetMachine &TM,
                                             const M65832Subtarget &STI)
    : TargetLowering(TM, STI), Subtarget(STI) {
  
  // Set up register classes - GPR for integers
  addRegisterClass(MVT::i32, &M65832::GPRRegClass);
  
  // FPU register classes for floating point
  addRegisterClass(MVT::f32, &M65832::FPR32RegClass);
  addRegisterClass(MVT::f64, &M65832::FPR64RegClass);
  
  // Compute derived properties from register classes
  // MUST be called after all register classes are added
  computeRegisterProperties(Subtarget.getRegisterInfo());
  
  // Set stack pointer register
  setStackPointerRegisterToSaveRestore(M65832::SP);
  
  // Set scheduling preference
  setSchedulingPreference(Sched::RegPressure);
  
  // Operations that need custom lowering
  setOperationAction(ISD::GlobalAddress, MVT::i32, Custom);
  setOperationAction(ISD::ExternalSymbol, MVT::i32, Custom);
  setOperationAction(ISD::BlockAddress, MVT::i32, Custom);
  setOperationAction(ISD::ConstantPool, MVT::i32, Custom);
  
  setOperationAction(ISD::BR_CC, MVT::i32, Custom);
  setOperationAction(ISD::BRCOND, MVT::Other, Expand);  // Expand to BR_CC with cmp against 0
  setOperationAction(ISD::SELECT_CC, MVT::i32, Custom);
  setOperationAction(ISD::SETCC, MVT::i32, Custom);
  
  // Basic ALU operations - we handle these by going through accumulator
  // For now, mark as Legal and let instruction selection handle them
  setOperationAction(ISD::ADD, MVT::i32, Legal);
  setOperationAction(ISD::SUB, MVT::i32, Legal);
  setOperationAction(ISD::AND, MVT::i32, Legal);
  setOperationAction(ISD::OR, MVT::i32, Legal);
  setOperationAction(ISD::XOR, MVT::i32, Legal);
  
  // Shifts - now have hardware barrel shifter!
  setOperationAction(ISD::SHL, MVT::i32, Legal);
  setOperationAction(ISD::SRL, MVT::i32, Legal);
  setOperationAction(ISD::SRA, MVT::i32, Legal);
  
  // Rotates - also supported by barrel shifter
  setOperationAction(ISD::ROTL, MVT::i32, Legal);
  setOperationAction(ISD::ROTR, MVT::i32, Legal);
  
  // Multi-word shifts (for 64-bit shifts on 32-bit target)
  // Custom lowering uses the barrel shifter for efficient implementation
  setOperationAction(ISD::SHL_PARTS, MVT::i32, Custom);
  setOperationAction(ISD::SRL_PARTS, MVT::i32, Custom);
  setOperationAction(ISD::SRA_PARTS, MVT::i32, Custom);
  
  setOperationAction(ISD::VASTART, MVT::Other, Custom);
  setOperationAction(ISD::VAARG, MVT::Other, Expand);
  setOperationAction(ISD::VACOPY, MVT::Other, Expand);
  setOperationAction(ISD::VAEND, MVT::Other, Expand);
  
  setOperationAction(ISD::FRAMEADDR, MVT::i32, Custom);
  setOperationAction(ISD::RETURNADDR, MVT::i32, Custom);
  
  // Expand complex operations
  setOperationAction(ISD::SDIV, MVT::i32, Subtarget.hasHWMul() ? Legal : Expand);
  setOperationAction(ISD::UDIV, MVT::i32, Subtarget.hasHWMul() ? Legal : Expand);
  setOperationAction(ISD::SREM, MVT::i32, Subtarget.hasHWMul() ? Legal : Expand);
  setOperationAction(ISD::UREM, MVT::i32, Subtarget.hasHWMul() ? Legal : Expand);
  setOperationAction(ISD::MUL, MVT::i32, Subtarget.hasHWMul() ? Legal : Expand);
  
  setOperationAction(ISD::MULHS, MVT::i32, Expand);
  setOperationAction(ISD::MULHU, MVT::i32, Expand);
  setOperationAction(ISD::SMUL_LOHI, MVT::i32, Expand);
  setOperationAction(ISD::UMUL_LOHI, MVT::i32, Expand);
  
  // Bit manipulation - now have hardware CLZ, CTZ, POPCNT!
  setOperationAction(ISD::CTLZ, MVT::i32, Legal);
  setOperationAction(ISD::CTTZ, MVT::i32, Legal);
  setOperationAction(ISD::CTPOP, MVT::i32, Legal);
  setOperationAction(ISD::BSWAP, MVT::i32, Expand);
  
  // Sign/zero extends - now have hardware SEXT8/SEXT16!
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i8, Legal);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i16, Legal);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i1, Expand);
  
  // Dynamic stack allocation not supported directly
  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i32, Expand);
  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);
  
  // We don't have conditional moves
  setOperationAction(ISD::SELECT, MVT::i32, Expand);
  
  // Jump tables and indirect branches - expand to cascading branches
  setOperationAction(ISD::BR_JT, MVT::Other, Expand);
  setOperationAction(ISD::BRIND, MVT::Other, Expand);
  
  // Minimum threshold for jump tables (effectively disable them)
  setMinimumJumpTableEntries(UINT_MAX);
  
  // Boolean values are i32
  setBooleanContents(ZeroOrOneBooleanContent);
  setBooleanVectorContents(ZeroOrOneBooleanContent);
  
  // Minimum function alignment
  setMinFunctionAlignment(Align(1));
  setPrefFunctionAlignment(Align(4));
  
  // Stack alignment
  setMinStackArgumentAlignment(Align(4));
  
  // =========================================================================
  // Load/Store Extension Actions
  // =========================================================================
  // Zero-extending loads: use extloadi8/extloadi16 patterns (LOAD8/LOAD16)
  // Sign-extending loads: expand to load + sign-extend
  for (MVT VT : MVT::integer_valuetypes()) {
    // EXTLOAD (any-extending) - Legal, matched by LOAD8/LOAD16 patterns
    setLoadExtAction(ISD::EXTLOAD, VT, MVT::i1, Promote);
    setLoadExtAction(ISD::EXTLOAD, VT, MVT::i8, Legal);
    setLoadExtAction(ISD::EXTLOAD, VT, MVT::i16, Legal);
    // ZEXTLOAD - expand to EXTLOAD + AND (if needed), or just use EXTLOAD
    setLoadExtAction(ISD::ZEXTLOAD, VT, MVT::i1, Promote);
    setLoadExtAction(ISD::ZEXTLOAD, VT, MVT::i8, Expand);
    setLoadExtAction(ISD::ZEXTLOAD, VT, MVT::i16, Expand);
    // SEXTLOAD - expand to EXTLOAD + sign-extend (SEXT8/SEXT16)
    setLoadExtAction(ISD::SEXTLOAD, VT, MVT::i1, Promote);
    setLoadExtAction(ISD::SEXTLOAD, VT, MVT::i8, Expand);
    setLoadExtAction(ISD::SEXTLOAD, VT, MVT::i16, Expand);
  }
  
  // Truncating stores - Legal, matched by STORE8/STORE16 patterns
  setTruncStoreAction(MVT::i32, MVT::i8, Legal);
  setTruncStoreAction(MVT::i32, MVT::i16, Legal);
  setTruncStoreAction(MVT::i16, MVT::i8, Legal);
  
  // =========================================================================
  // Floating Point Support
  // =========================================================================
  // M65832 has an FPU with sixteen 64-bit registers (F0-F15).
  // Operations use two-operand destructive format: Fd = Fd op Fs
  // Uses hard-float ABI: floats passed in F0-F7, returned in F0.
  // Note: FPU register classes added above before computeRegisterProperties()
  
  // Basic FP operations - Legal (matched by TableGen patterns)
  setOperationAction(ISD::FADD, MVT::f32, Legal);
  setOperationAction(ISD::FSUB, MVT::f32, Legal);
  setOperationAction(ISD::FMUL, MVT::f32, Legal);
  setOperationAction(ISD::FDIV, MVT::f32, Legal);
  setOperationAction(ISD::FNEG, MVT::f32, Legal);
  setOperationAction(ISD::FABS, MVT::f32, Legal);
  setOperationAction(ISD::FSQRT, MVT::f32, Legal);
  
  setOperationAction(ISD::FADD, MVT::f64, Legal);
  setOperationAction(ISD::FSUB, MVT::f64, Legal);
  setOperationAction(ISD::FMUL, MVT::f64, Legal);
  setOperationAction(ISD::FDIV, MVT::f64, Legal);
  setOperationAction(ISD::FNEG, MVT::f64, Legal);
  setOperationAction(ISD::FABS, MVT::f64, Legal);
  setOperationAction(ISD::FSQRT, MVT::f64, Legal);
  
  // FP conversions - Legal (FCVT.DS, FCVT.SD, F2I, I2F)
  setOperationAction(ISD::FP_EXTEND, MVT::f64, Legal);
  setOperationAction(ISD::FP_ROUND, MVT::f32, Legal);
  setOperationAction(ISD::FP_TO_SINT, MVT::i32, Legal);
  setOperationAction(ISD::SINT_TO_FP, MVT::i32, Legal);
  
  // Unsigned conversions - expand (no direct hardware support)
  setOperationAction(ISD::FP_TO_UINT, MVT::i32, Expand);
  setOperationAction(ISD::UINT_TO_FP, MVT::i32, Expand);
  setOperationAction(ISD::FP_TO_SINT, MVT::i64, Expand);
  setOperationAction(ISD::FP_TO_UINT, MVT::i64, Expand);
  setOperationAction(ISD::SINT_TO_FP, MVT::i64, Expand);
  setOperationAction(ISD::UINT_TO_FP, MVT::i64, Expand);
  
  // FP load/store - Legal (use LDF/STF instructions)
  setOperationAction(ISD::LOAD, MVT::f32, Legal);
  setOperationAction(ISD::LOAD, MVT::f64, Legal);
  setOperationAction(ISD::STORE, MVT::f32, Legal);
  setOperationAction(ISD::STORE, MVT::f64, Legal);
  
  // FP extending loads - expand to load + fpextend
  // (Can't do extending load directly, need separate load and convert)
  setLoadExtAction(ISD::EXTLOAD, MVT::f64, MVT::f32, Expand);
  
  // FP/Int bitcast - expand through memory (no direct FMV instruction)
  setOperationAction(ISD::BITCAST, MVT::f32, Expand);
  setOperationAction(ISD::BITCAST, MVT::i32, Expand);
  setOperationAction(ISD::BITCAST, MVT::f64, Expand);
  setOperationAction(ISD::BITCAST, MVT::i64, Expand);
  
  // FP comparisons - Custom lowering using FCMP + conditional select
  setOperationAction(ISD::SETCC, MVT::f32, Custom);
  setOperationAction(ISD::SETCC, MVT::f64, Custom);
  setOperationAction(ISD::BR_CC, MVT::f32, Custom);
  setOperationAction(ISD::BR_CC, MVT::f64, Custom);
  
  // Operations NOT supported by hardware - expand to libcalls
  setOperationAction(ISD::FREM, MVT::f32, Expand);
  setOperationAction(ISD::FREM, MVT::f64, Expand);
  setOperationAction(ISD::FSIN, MVT::f32, Expand);
  setOperationAction(ISD::FSIN, MVT::f64, Expand);
  setOperationAction(ISD::FCOS, MVT::f32, Expand);
  setOperationAction(ISD::FCOS, MVT::f64, Expand);
  setOperationAction(ISD::FPOW, MVT::f32, Expand);
  setOperationAction(ISD::FPOW, MVT::f64, Expand);
  setOperationAction(ISD::FLOG, MVT::f32, Expand);
  setOperationAction(ISD::FLOG, MVT::f64, Expand);
  setOperationAction(ISD::FLOG2, MVT::f32, Expand);
  setOperationAction(ISD::FLOG2, MVT::f64, Expand);
  setOperationAction(ISD::FLOG10, MVT::f32, Expand);
  setOperationAction(ISD::FLOG10, MVT::f64, Expand);
  setOperationAction(ISD::FEXP, MVT::f32, Expand);
  setOperationAction(ISD::FEXP, MVT::f64, Expand);
  setOperationAction(ISD::FEXP2, MVT::f32, Expand);
  setOperationAction(ISD::FEXP2, MVT::f64, Expand);
  setOperationAction(ISD::FEXP10, MVT::f32, Expand);
  setOperationAction(ISD::FEXP10, MVT::f64, Expand);
  setOperationAction(ISD::FCEIL, MVT::f32, Expand);
  setOperationAction(ISD::FCEIL, MVT::f64, Expand);
  setOperationAction(ISD::FFLOOR, MVT::f32, Expand);
  setOperationAction(ISD::FFLOOR, MVT::f64, Expand);
  setOperationAction(ISD::FTRUNC, MVT::f32, Expand);
  setOperationAction(ISD::FTRUNC, MVT::f64, Expand);
  setOperationAction(ISD::FRINT, MVT::f32, Expand);
  setOperationAction(ISD::FRINT, MVT::f64, Expand);
  setOperationAction(ISD::FNEARBYINT, MVT::f32, Expand);
  setOperationAction(ISD::FNEARBYINT, MVT::f64, Expand);
  setOperationAction(ISD::FROUND, MVT::f32, Expand);
  setOperationAction(ISD::FROUND, MVT::f64, Expand);
  setOperationAction(ISD::FROUNDEVEN, MVT::f32, Expand);
  setOperationAction(ISD::FROUNDEVEN, MVT::f64, Expand);
  setOperationAction(ISD::FCOPYSIGN, MVT::f32, Expand);
  setOperationAction(ISD::FCOPYSIGN, MVT::f64, Expand);
  setOperationAction(ISD::FMINNUM, MVT::f32, Expand);
  setOperationAction(ISD::FMINNUM, MVT::f64, Expand);
  setOperationAction(ISD::FMAXNUM, MVT::f32, Expand);
  setOperationAction(ISD::FMAXNUM, MVT::f64, Expand);
  setOperationAction(ISD::FMINIMUM, MVT::f32, Expand);
  setOperationAction(ISD::FMINIMUM, MVT::f64, Expand);
  setOperationAction(ISD::FMAXIMUM, MVT::f32, Expand);
  setOperationAction(ISD::FMAXIMUM, MVT::f64, Expand);
  setOperationAction(ISD::FMA, MVT::f32, Expand);
  setOperationAction(ISD::FMA, MVT::f64, Expand);
  setOperationAction(ISD::FMAD, MVT::f32, Expand);
  setOperationAction(ISD::FMAD, MVT::f64, Expand);
  
  // Common FP settings
  // For floating-point, we use Custom lowering for SELECT_CC and SELECT
  // These are expanded via EmitInstrWithCustomInserter into branch sequences
  setOperationAction(ISD::SELECT_CC, MVT::f32, Custom);
  setOperationAction(ISD::SELECT_CC, MVT::f64, Custom);
  // SELECT for FP types also needs Custom (can't use Expand if SELECT_CC is Custom)
  setOperationAction(ISD::SELECT, MVT::f32, Custom);
  setOperationAction(ISD::SELECT, MVT::f64, Custom);
  
  // Floating point constants - always expand (load from constant pool)
  setOperationAction(ISD::ConstantFP, MVT::f32, Expand);
  setOperationAction(ISD::ConstantFP, MVT::f64, Expand);
  
  // Truncating stores for FP - expand to convert + store
  setTruncStoreAction(MVT::f64, MVT::f32, Expand);
}

SDValue M65832TargetLowering::LowerOperation(SDValue Op,
                                               SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
  case ISD::GlobalAddress:    return LowerGlobalAddress(Op, DAG);
  case ISD::ExternalSymbol:   return LowerExternalSymbol(Op, DAG);
  case ISD::BlockAddress:     return LowerBlockAddress(Op, DAG);
  case ISD::ConstantPool:     return LowerConstantPool(Op, DAG);
  case ISD::BR_CC:            return LowerBR_CC(Op, DAG);
  case ISD::SELECT_CC:        return LowerSELECT_CC(Op, DAG);
  case ISD::SETCC:            return LowerSETCC(Op, DAG);
  case ISD::VASTART:          return LowerVASTART(Op, DAG);
  case ISD::FRAMEADDR:        return LowerFRAMEADDR(Op, DAG);
  case ISD::RETURNADDR:       return LowerRETURNADDR(Op, DAG);
  case ISD::SHL_PARTS:        return LowerShiftLeftParts(Op, DAG);
  case ISD::SRL_PARTS:        return LowerShiftRightParts(Op, DAG, false);
  case ISD::SRA_PARTS:        return LowerShiftRightParts(Op, DAG, true);
  case ISD::SELECT:           return LowerSELECT(Op, DAG);
  default:
    llvm_unreachable("unimplemented operation");
  }
}

const char *M65832TargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (static_cast<M65832ISD::NodeType>(Opcode)) {
  case M65832ISD::FIRST_NUMBER: break;
  case M65832ISD::RET_FLAG:     return "M65832ISD::RET_FLAG";
  case M65832ISD::CALL:         return "M65832ISD::CALL";
  case M65832ISD::CMP:          return "M65832ISD::CMP";
  case M65832ISD::FCMP:         return "M65832ISD::FCMP";
  case M65832ISD::BR_CC:        return "M65832ISD::BR_CC";
  case M65832ISD::BR_CC_CMP:    return "M65832ISD::BR_CC_CMP";
  case M65832ISD::SELECT_CC:    return "M65832ISD::SELECT_CC";
  case M65832ISD::SELECT_CC_MIXED: return "M65832ISD::SELECT_CC_MIXED";
  case M65832ISD::SELECT_CC_FP: return "M65832ISD::SELECT_CC_FP";
  case M65832ISD::WRAPPER:      return "M65832ISD::WRAPPER";
  case M65832ISD::SMUL_LOHI:    return "M65832ISD::SMUL_LOHI";
  case M65832ISD::UMUL_LOHI:    return "M65832ISD::UMUL_LOHI";
  case M65832ISD::SDIVREM:      return "M65832ISD::SDIVREM";
  case M65832ISD::UDIVREM:      return "M65832ISD::UDIVREM";
  }
  return nullptr;
}

SDValue M65832TargetLowering::LowerGlobalAddress(SDValue Op,
                                                   SelectionDAG &DAG) const {
  SDLoc DL(Op);
  const GlobalAddressSDNode *GN = cast<GlobalAddressSDNode>(Op);
  const GlobalValue *GV = GN->getGlobal();
  int64_t Offset = GN->getOffset();
  
  SDValue GA = DAG.getTargetGlobalAddress(GV, DL, MVT::i32, Offset);
  return DAG.getNode(M65832ISD::WRAPPER, DL, MVT::i32, GA);
}

SDValue M65832TargetLowering::LowerExternalSymbol(SDValue Op,
                                                    SelectionDAG &DAG) const {
  SDLoc DL(Op);
  const ExternalSymbolSDNode *ES = cast<ExternalSymbolSDNode>(Op);
  
  SDValue Symbol = DAG.getTargetExternalSymbol(ES->getSymbol(), MVT::i32);
  return DAG.getNode(M65832ISD::WRAPPER, DL, MVT::i32, Symbol);
}

SDValue M65832TargetLowering::LowerBlockAddress(SDValue Op,
                                                  SelectionDAG &DAG) const {
  SDLoc DL(Op);
  const BlockAddressSDNode *BA = cast<BlockAddressSDNode>(Op);
  
  SDValue Addr = DAG.getTargetBlockAddress(BA->getBlockAddress(), MVT::i32);
  return DAG.getNode(M65832ISD::WRAPPER, DL, MVT::i32, Addr);
}

SDValue M65832TargetLowering::LowerConstantPool(SDValue Op,
                                                  SelectionDAG &DAG) const {
  SDLoc DL(Op);
  const ConstantPoolSDNode *CP = cast<ConstantPoolSDNode>(Op);
  
  SDValue Addr;
  if (CP->isMachineConstantPoolEntry())
    Addr = DAG.getTargetConstantPool(CP->getMachineCPVal(), MVT::i32,
                                     CP->getAlign(), CP->getOffset());
  else
    Addr = DAG.getTargetConstantPool(CP->getConstVal(), MVT::i32,
                                     CP->getAlign(), CP->getOffset());
  
  return DAG.getNode(M65832ISD::WRAPPER, DL, MVT::i32, Addr);
}

SDValue M65832TargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  SDValue Chain = Op.getOperand(0);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
  SDValue LHS = Op.getOperand(2);
  SDValue RHS = Op.getOperand(3);
  SDValue Dest = Op.getOperand(4);
  
  EVT CmpVT = LHS.getValueType();
  if (CmpVT == MVT::f32 || CmpVT == MVT::f64) {
    SDValue Cmp = DAG.getNode(M65832ISD::FCMP, DL, MVT::Glue, LHS, RHS);
    SDValue CCVal = DAG.getConstant(CC, DL, MVT::i32);
    return DAG.getNode(M65832ISD::BR_CC, DL, Op.getValueType(), Chain, Dest,
                       CCVal, Cmp);
  }

  // Canonicalize to avoid SETGT/SETLE/SETUGT/SETULE in fused branch
  switch (CC) {
  case ISD::SETGT:
    CC = ISD::SETLT;
    std::swap(LHS, RHS);
    break;
  case ISD::SETLE:
    CC = ISD::SETGE;
    std::swap(LHS, RHS);
    break;
  case ISD::SETUGT:
    CC = ISD::SETULT;
    std::swap(LHS, RHS);
    break;
  case ISD::SETULE:
    CC = ISD::SETUGE;
    std::swap(LHS, RHS);
    break;
  default:
    break;
  }

  // For integers, use fused compare-and-branch to prevent flag clobbering
  SDValue CCVal = DAG.getConstant(CC, DL, MVT::i32);
  return DAG.getNode(M65832ISD::BR_CC_CMP, DL, Op.getValueType(), Chain,
                     LHS, RHS, CCVal, Dest);
}

SDValue M65832TargetLowering::LowerSELECT_CC(SDValue Op,
                                               SelectionDAG &DAG) const {
  SDLoc DL(Op);
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue TrueVal = Op.getOperand(2);
  SDValue FalseVal = Op.getOperand(3);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(4))->get();
  EVT CmpVT = LHS.getValueType();
  
  // For floating point comparisons, use FCMP with glue-based SELECT_CC_FP
  // FP operations don't have the flag-clobbering issue that integer ops do
  if (CmpVT == MVT::f32 || CmpVT == MVT::f64) {
    SDValue Cmp = DAG.getNode(M65832ISD::FCMP, DL, MVT::Glue, LHS, RHS);
    SDValue CCVal = DAG.getConstant(CC, DL, MVT::i32);
    return DAG.getNode(M65832ISD::SELECT_CC_FP, DL, Op.getValueType(), 
                       TrueVal, FalseVal, CCVal, Cmp);
  }
  
  // Canonicalize integer comparisons to avoid SETGT/SETLE/SETUGT/SETULE
  switch (CC) {
  case ISD::SETGT:
    CC = ISD::SETLT;
    std::swap(LHS, RHS);
    break;
  case ISD::SETLE:
    CC = ISD::SETGE;
    std::swap(LHS, RHS);
    break;
  case ISD::SETUGT:
    CC = ISD::SETULT;
    std::swap(LHS, RHS);
    break;
  case ISD::SETULE:
    CC = ISD::SETUGE;
    std::swap(LHS, RHS);
    break;
  default:
    break;
  }

  // For integers, include LHS/RHS so each SELECT has its own CMP
  // This ensures flags aren't clobbered by intervening instructions
  SDValue CCVal = DAG.getConstant(CC, DL, MVT::i32);
  
  // If result type is FP but comparison is integer, use SELECT_CC_MIXED
  EVT ResultVT = Op.getValueType();
  unsigned Opc = ResultVT.isFloatingPoint() ? M65832ISD::SELECT_CC_MIXED 
                                             : M65832ISD::SELECT_CC;
  
  return DAG.getNode(Opc, DL, ResultVT, LHS, RHS, TrueVal, FalseVal, CCVal);
}

SDValue M65832TargetLowering::LowerSETCC(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(2))->get();
  EVT CmpVT = LHS.getValueType();
  
  SDValue One = DAG.getConstant(1, DL, MVT::i32);
  SDValue Zero = DAG.getConstant(0, DL, MVT::i32);
  SDValue CCVal = DAG.getConstant(CC, DL, MVT::i32);
  
  // For floating point comparisons, use FCMP with glue-based SELECT_CC_FP
  if (CmpVT == MVT::f32 || CmpVT == MVT::f64) {
    SDValue Cmp = DAG.getNode(M65832ISD::FCMP, DL, MVT::Glue, LHS, RHS);
    return DAG.getNode(M65832ISD::SELECT_CC_FP, DL, MVT::i32, 
                       One, Zero, CCVal, Cmp);
  }
  
  // Canonicalize integer comparisons to avoid SETGT/SETLE/SETUGT/SETULE
  switch (CC) {
  case ISD::SETGT:
    CC = ISD::SETLT;
    std::swap(LHS, RHS);
    break;
  case ISD::SETLE:
    CC = ISD::SETGE;
    std::swap(LHS, RHS);
    break;
  case ISD::SETUGT:
    CC = ISD::SETULT;
    std::swap(LHS, RHS);
    break;
  case ISD::SETULE:
    CC = ISD::SETUGE;
    std::swap(LHS, RHS);
    break;
  default:
    break;
  }

  // For integers, include LHS/RHS for comparison
  CCVal = DAG.getConstant(CC, DL, MVT::i32);
  return DAG.getNode(M65832ISD::SELECT_CC, DL, MVT::i32, 
                     LHS, RHS, One, Zero, CCVal);
}

SDValue M65832TargetLowering::LowerSELECT(SDValue Op, SelectionDAG &DAG) const {
  SDLoc DL(Op);
  SDValue Cond = Op.getOperand(0);
  SDValue TrueVal = Op.getOperand(1);
  SDValue FalseVal = Op.getOperand(2);
  EVT VT = Op.getValueType();
  
  // Convert SELECT to SELECT_CC by comparing the condition against 0
  // This pattern: select(cond, tv, fv) -> select_cc(cond, 0, tv, fv, NE)
  SDValue Zero = DAG.getConstant(0, DL, Cond.getValueType());
  SDValue CCVal = DAG.getConstant(ISD::SETNE, DL, MVT::i32);
  
  // For integer results, use SELECT_CC; for FP results, use SELECT_CC_MIXED
  // SELECT_CC_MIXED allows integer comparison with FP result
  unsigned Opc = VT.isFloatingPoint() ? M65832ISD::SELECT_CC_MIXED 
                                       : M65832ISD::SELECT_CC;
  
  return DAG.getNode(Opc, DL, VT, Cond, Zero, TrueVal, FalseVal, CCVal);
}

SDValue M65832TargetLowering::LowerVASTART(SDValue Op, SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  M65832MachineFunctionInfo *FuncInfo = MF.getInfo<M65832MachineFunctionInfo>();
  
  SDLoc DL(Op);
  SDValue Chain = Op.getOperand(0);
  SDValue VAListPtr = Op.getOperand(1);
  
  // VA list is a pointer to the first vararg on stack
  SDValue FrameIndex = DAG.getFrameIndex(FuncInfo->getVarArgsFrameIndex(),
                                         MVT::i32);
  
  const Value *SV = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  return DAG.getStore(Chain, DL, FrameIndex, VAListPtr,
                      MachinePointerInfo(SV));
}

SDValue M65832TargetLowering::LowerFRAMEADDR(SDValue Op,
                                               SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MFI.setFrameAddressIsTaken(true);
  
  SDLoc DL(Op);
  unsigned Depth = cast<ConstantSDNode>(Op.getOperand(0))->getZExtValue();
  
  if (Depth > 0) {
    // For non-zero depth, we need to walk the frame chain
    report_fatal_error("Non-zero frame depth not supported");
  }
  
  // Return R29 (frame pointer)
  return DAG.getCopyFromReg(DAG.getEntryNode(), DL, M65832::R29, MVT::i32);
}

SDValue M65832TargetLowering::LowerRETURNADDR(SDValue Op,
                                                SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MFI.setReturnAddressIsTaken(true);
  
  unsigned Depth = cast<ConstantSDNode>(Op.getOperand(0))->getZExtValue();
  SDLoc DL(Op);
  
  if (Depth > 0)
    report_fatal_error("Non-zero return address depth not supported");
  
  // Return address is stored by JSR at [SP]
  // This is complex on M65832, we'd need to access the stack
  return DAG.getUNDEF(MVT::i32);
}

// Lower 64-bit shift left on 32-bit target using barrel shifter
// SHL_PARTS: (Lo, Hi) = SHL_PARTS(LoIn, HiIn, ShiftAmt)
SDValue M65832TargetLowering::LowerShiftLeftParts(SDValue Op,
                                                   SelectionDAG &DAG) const {
  SDLoc DL(Op);
  SDValue Lo = Op.getOperand(0);
  SDValue Hi = Op.getOperand(1);
  SDValue Shamt = Op.getOperand(2);
  EVT VT = Lo.getValueType();

  // if Shamt < 32:
  //   Lo = Lo << Shamt
  //   Hi = (Hi << Shamt) | (Lo >> (32 - Shamt))
  // else:
  //   Lo = 0
  //   Hi = Lo << (Shamt - 32)
  
  SDValue Zero = DAG.getConstant(0, DL, VT);
  SDValue One = DAG.getConstant(1, DL, VT);
  SDValue Minus32 = DAG.getSignedConstant(-32, DL, VT);
  SDValue ThirtyOne = DAG.getConstant(31, DL, VT);
  
  SDValue ShamtMinus32 = DAG.getNode(ISD::ADD, DL, VT, Shamt, Minus32);
  SDValue ThirtyOneMinusShamt = DAG.getNode(ISD::SUB, DL, VT, ThirtyOne, Shamt);

  SDValue LoTrue = DAG.getNode(ISD::SHL, DL, VT, Lo, Shamt);
  SDValue ShiftRight1Lo = DAG.getNode(ISD::SRL, DL, VT, Lo, One);
  SDValue ShiftRightLo = DAG.getNode(ISD::SRL, DL, VT, ShiftRight1Lo, ThirtyOneMinusShamt);
  SDValue ShiftLeftHi = DAG.getNode(ISD::SHL, DL, VT, Hi, Shamt);
  SDValue HiTrue = DAG.getNode(ISD::OR, DL, VT, ShiftLeftHi, ShiftRightLo);
  SDValue HiFalse = DAG.getNode(ISD::SHL, DL, VT, Lo, ShamtMinus32);

  SDValue CC = DAG.getSetCC(DL, VT, ShamtMinus32, Zero, ISD::SETLT);

  Lo = DAG.getNode(ISD::SELECT, DL, VT, CC, LoTrue, Zero);
  Hi = DAG.getNode(ISD::SELECT, DL, VT, CC, HiTrue, HiFalse);

  SDValue Parts[2] = {Lo, Hi};
  return DAG.getMergeValues(Parts, DL);
}

// Lower 64-bit shift right on 32-bit target using barrel shifter
// SRL_PARTS/SRA_PARTS: (Lo, Hi) = SRL_PARTS(LoIn, HiIn, ShiftAmt)
SDValue M65832TargetLowering::LowerShiftRightParts(SDValue Op,
                                                    SelectionDAG &DAG,
                                                    bool IsSRA) const {
  SDLoc DL(Op);
  SDValue Lo = Op.getOperand(0);
  SDValue Hi = Op.getOperand(1);
  SDValue Shamt = Op.getOperand(2);
  EVT VT = Lo.getValueType();

  // SRA expansion:
  //   if Shamt < 32:
  //     Lo = (Lo >> Shamt) | (Hi << (32 - Shamt))
  //     Hi = Hi >> Shamt (arithmetic)
  //   else:
  //     Lo = Hi >> (Shamt - 32) (arithmetic)
  //     Hi = Hi >> 31 (sign extension)
  //
  // SRL expansion:
  //   if Shamt < 32:
  //     Lo = (Lo >> Shamt) | (Hi << (32 - Shamt))
  //     Hi = Hi >> Shamt (logical)
  //   else:
  //     Lo = Hi >> (Shamt - 32) (logical)
  //     Hi = 0

  unsigned ShiftRightOp = IsSRA ? ISD::SRA : ISD::SRL;

  SDValue Zero = DAG.getConstant(0, DL, VT);
  SDValue One = DAG.getConstant(1, DL, VT);
  SDValue Minus32 = DAG.getSignedConstant(-32, DL, VT);
  SDValue ThirtyOne = DAG.getConstant(31, DL, VT);
  
  SDValue ShamtMinus32 = DAG.getNode(ISD::ADD, DL, VT, Shamt, Minus32);
  SDValue ThirtyOneMinusShamt = DAG.getNode(ISD::SUB, DL, VT, ThirtyOne, Shamt);

  SDValue ShiftRightLo = DAG.getNode(ISD::SRL, DL, VT, Lo, Shamt);
  SDValue ShiftLeftHi1 = DAG.getNode(ISD::SHL, DL, VT, Hi, One);
  SDValue ShiftLeftHi = DAG.getNode(ISD::SHL, DL, VT, ShiftLeftHi1, ThirtyOneMinusShamt);
  SDValue LoTrue = DAG.getNode(ISD::OR, DL, VT, ShiftRightLo, ShiftLeftHi);
  SDValue HiTrue = DAG.getNode(ShiftRightOp, DL, VT, Hi, Shamt);
  SDValue LoFalse = DAG.getNode(ShiftRightOp, DL, VT, Hi, ShamtMinus32);
  SDValue HiFalse = IsSRA ? DAG.getNode(ISD::SRA, DL, VT, Hi, ThirtyOne) : Zero;

  SDValue CC = DAG.getSetCC(DL, VT, ShamtMinus32, Zero, ISD::SETLT);

  Lo = DAG.getNode(ISD::SELECT, DL, VT, CC, LoTrue, LoFalse);
  Hi = DAG.getNode(ISD::SELECT, DL, VT, CC, HiTrue, HiFalse);

  SDValue Parts[2] = {Lo, Hi};
  return DAG.getMergeValues(Parts, DL);
}

Register M65832TargetLowering::getRegisterByName(const char *RegName, LLT VT,
                                                   const MachineFunction &MF) const {
  // Handle named registers
  if (StringRef(RegName) == "sp")
    return M65832::SP;
  
  report_fatal_error("Invalid register name");
}

// Calling convention implementation

SDValue M65832TargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  M65832MachineFunctionInfo *FuncInfo = MF.getInfo<M65832MachineFunctionInfo>();
  
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CC_M65832);
  
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    
    if (VA.isRegLoc()) {
      // Argument passed in register
      EVT RegVT = VA.getLocVT();
      MCRegister LocReg = VA.getLocReg();
      
      // Select the correct register class based on register type
      const TargetRegisterClass *RC;
      if (M65832::FPR32RegClass.contains(LocReg) ||
          M65832::FPR64RegClass.contains(LocReg)) {
        RC = RegVT == MVT::f64 ? &M65832::FPR64RegClass : &M65832::FPR32RegClass;
      } else {
        RC = &M65832::GPRRegClass;
      }
      
      // Add the register as a live-in
      Register Reg = MF.addLiveIn(LocReg, RC);
      
      SDValue ArgValue = DAG.getCopyFromReg(Chain, DL, Reg, RegVT);
      
      // Handle any necessary conversions
      if (VA.getLocInfo() == CCValAssign::SExt)
        ArgValue = DAG.getNode(ISD::AssertSext, DL, RegVT, ArgValue,
                               DAG.getValueType(VA.getValVT()));
      else if (VA.getLocInfo() == CCValAssign::ZExt)
        ArgValue = DAG.getNode(ISD::AssertZext, DL, RegVT, ArgValue,
                               DAG.getValueType(VA.getValVT()));
      
      if (VA.getLocInfo() != CCValAssign::Full)
        ArgValue = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), ArgValue);
      
      InVals.push_back(ArgValue);
    } else {
      // Argument passed on stack
      assert(VA.isMemLoc() && "Must be memory location");
      
      int FI = MFI.CreateFixedObject(VA.getLocVT().getSizeInBits() / 8,
                                     VA.getLocMemOffset(), true);
      SDValue FIN = DAG.getFrameIndex(FI, MVT::i32);
      SDValue Load = DAG.getLoad(VA.getLocVT(), DL, Chain, FIN,
                                 MachinePointerInfo::getFixedStack(MF, FI));
      InVals.push_back(Load);
    }
  }
  
  if (isVarArg) {
    // Save the position of first vararg for va_start
    unsigned FirstVarArg = CCInfo.getStackSize();
    FuncInfo->setVarArgsFrameIndex(
        MFI.CreateFixedObject(4, FirstVarArg, true));
  }
  
  return Chain;
}

SDValue M65832TargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                                          SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG = CLI.DAG;
  SDLoc &DL = CLI.DL;
  SmallVectorImpl<ISD::OutputArg> &Outs = CLI.Outs;
  SmallVectorImpl<SDValue> &OutVals = CLI.OutVals;
  SmallVectorImpl<ISD::InputArg> &Ins = CLI.Ins;
  SDValue Chain = CLI.Chain;
  SDValue Callee = CLI.Callee;
  CallingConv::ID CallConv = CLI.CallConv;
  bool isVarArg = CLI.IsVarArg;
  
  // Disable tail call optimization - not properly implemented yet
  CLI.IsTailCall = false;
  
  MachineFunction &MF = DAG.getMachineFunction();
  
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeCallOperands(Outs, CC_M65832);
  
  unsigned StackSize = CCInfo.getStackSize();
  
  // M65832 JSR pushes a 4-byte return address onto the stack in 32-bit mode.
  // We must reserve space for this even when there are no stack-passed arguments.
  // Without this, local variables on the stack would be corrupted by JSR.
  unsigned CallFrameSize = std::max(StackSize, 4u);
  
  // Adjust stack
  Chain = DAG.getCALLSEQ_START(Chain, CallFrameSize, 0, DL);
  
  SmallVector<std::pair<unsigned, SDValue>, 8> RegsToPass;
  SmallVector<SDValue, 8> MemOpChains;
  
  // Process arguments
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    SDValue Arg = OutVals[i];
    
    // Promote if necessary
    switch (VA.getLocInfo()) {
    case CCValAssign::Full:
      break;
    case CCValAssign::SExt:
      Arg = DAG.getNode(ISD::SIGN_EXTEND, DL, VA.getLocVT(), Arg);
      break;
    case CCValAssign::ZExt:
      Arg = DAG.getNode(ISD::ZERO_EXTEND, DL, VA.getLocVT(), Arg);
      break;
    case CCValAssign::AExt:
      Arg = DAG.getNode(ISD::ANY_EXTEND, DL, VA.getLocVT(), Arg);
      break;
    default:
      llvm_unreachable("Unknown loc info");
    }
    
    if (VA.isRegLoc()) {
      RegsToPass.push_back(std::make_pair(VA.getLocReg(), Arg));
    } else {
      assert(VA.isMemLoc() && "Must be mem loc");
      SDValue StackPtr = DAG.getCopyFromReg(Chain, DL, M65832::SP, MVT::i32);
      SDValue PtrOff = DAG.getIntPtrConstant(VA.getLocMemOffset(), DL);
      PtrOff = DAG.getNode(ISD::ADD, DL, MVT::i32, StackPtr, PtrOff);
      MemOpChains.push_back(
          DAG.getStore(Chain, DL, Arg, PtrOff, MachinePointerInfo()));
    }
  }
  
  if (!MemOpChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, MemOpChains);
  
  // Build list of register copies
  SDValue InGlue;
  for (auto &Reg : RegsToPass) {
    Chain = DAG.getCopyToReg(Chain, DL, Reg.first, Reg.second, InGlue);
    InGlue = Chain.getValue(1);
  }
  
  // Get callee address
  if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee))
    Callee = DAG.getTargetGlobalAddress(G->getGlobal(), DL, MVT::i32);
  else if (ExternalSymbolSDNode *E = dyn_cast<ExternalSymbolSDNode>(Callee))
    Callee = DAG.getTargetExternalSymbol(E->getSymbol(), MVT::i32);
  
  // Build call
  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Chain);
  Ops.push_back(Callee);
  
  for (auto &Reg : RegsToPass)
    Ops.push_back(DAG.getRegister(Reg.first, Reg.second.getValueType()));
  
  if (InGlue.getNode())
    Ops.push_back(InGlue);
  
  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
  Chain = DAG.getNode(M65832ISD::CALL, DL, NodeTys, Ops);
  InGlue = Chain.getValue(1);
  
  // Adjust stack back
  Chain = DAG.getCALLSEQ_END(Chain, CallFrameSize, 0, InGlue, DL);
  InGlue = Chain.getValue(1);
  
  // Handle return values
  SmallVector<CCValAssign, 16> RVLocs;
  CCState RVInfo(CallConv, isVarArg, MF, RVLocs, *DAG.getContext());
  RVInfo.AnalyzeCallResult(Ins, RetCC_M65832);
  
  for (unsigned i = 0, e = RVLocs.size(); i != e; ++i) {
    CCValAssign &VA = RVLocs[i];
    SDValue Val = DAG.getCopyFromReg(Chain, DL, VA.getLocReg(),
                                     VA.getLocVT(), InGlue);
    Chain = Val.getValue(1);
    InGlue = Val.getValue(2);
    
    if (VA.getLocInfo() == CCValAssign::SExt)
      Val = DAG.getNode(ISD::AssertSext, DL, VA.getLocVT(), Val,
                        DAG.getValueType(VA.getValVT()));
    else if (VA.getLocInfo() == CCValAssign::ZExt)
      Val = DAG.getNode(ISD::AssertZext, DL, VA.getLocVT(), Val,
                        DAG.getValueType(VA.getValVT()));
    
    if (VA.getLocInfo() != CCValAssign::Full)
      Val = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), Val);
    
    InVals.push_back(Val);
  }
  
  return Chain;
}

SDValue M65832TargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CallConv,
                                            bool isVarArg,
                                            const SmallVectorImpl<ISD::OutputArg> &Outs,
                                            const SmallVectorImpl<SDValue> &OutVals,
                                            const SDLoc &DL,
                                            SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, isVarArg, MF, RVLocs, *DAG.getContext());
  CCInfo.AnalyzeReturn(Outs, RetCC_M65832);
  
  SDValue Glue;
  SmallVector<SDValue, 4> RetOps(1, Chain);
  
  // Copy return values to registers
  for (unsigned i = 0, e = RVLocs.size(); i != e; ++i) {
    CCValAssign &VA = RVLocs[i];
    SDValue Val = OutVals[i];
    
    // Promote if necessary
    switch (VA.getLocInfo()) {
    case CCValAssign::Full:
      break;
    case CCValAssign::SExt:
      Val = DAG.getNode(ISD::SIGN_EXTEND, DL, VA.getLocVT(), Val);
      break;
    case CCValAssign::ZExt:
      Val = DAG.getNode(ISD::ZERO_EXTEND, DL, VA.getLocVT(), Val);
      break;
    case CCValAssign::AExt:
      Val = DAG.getNode(ISD::ANY_EXTEND, DL, VA.getLocVT(), Val);
      break;
    default:
      llvm_unreachable("Unknown loc info");
    }
    
    Chain = DAG.getCopyToReg(Chain, DL, VA.getLocReg(), Val, Glue);
    Glue = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }
  
  RetOps[0] = Chain;
  if (Glue.getNode())
    RetOps.push_back(Glue);
  
  return DAG.getNode(M65832ISD::RET_FLAG, DL, MVT::Other, RetOps);
}

bool M65832TargetLowering::CanLowerReturn(
    CallingConv::ID CallConv, MachineFunction &MF, bool isVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context,
    const Type *RetTy) const {
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, isVarArg, MF, RVLocs, Context);
  return CCInfo.CheckReturn(Outs, RetCC_M65832);
}

MachineBasicBlock *
M65832TargetLowering::EmitInstrWithCustomInserter(MachineInstr &MI,
                                                    MachineBasicBlock *MBB) const {
  switch (MI.getOpcode()) {
  case M65832::SELECT:
  case M65832::SELECT_F32:
  case M65832::SELECT_F64:
    return emitSelect(MI, MBB);
  case M65832::SELECT_CC_PSEUDO:
  case M65832::SELECT_CC_F32_PSEUDO:
  case M65832::SELECT_CC_F64_PSEUDO:
    return emitSelectCC(MI, MBB);
  case M65832::SELECT_CC_FP_PSEUDO:
  case M65832::SELECT_CC_FP_F32_PSEUDO:
  case M65832::SELECT_CC_FP_F64_PSEUDO:
    return emitSelectCCFP(MI, MBB);
  default:
    llvm_unreachable("Unexpected instruction for custom inserter");
  }
}

MachineBasicBlock *M65832TargetLowering::emitSelect(MachineInstr &MI,
                                                      MachineBasicBlock *MBB) const {
  // This implements the SELECT pseudo by expanding to PHI nodes
  // The condition flags are already set before this instruction
  const TargetInstrInfo &TII = *Subtarget.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  
  // Determine if this is a FP select
  bool IsFP = (MI.getOpcode() == M65832::SELECT_F32 || 
               MI.getOpcode() == M65832::SELECT_F64);
  
  // Create new basic blocks
  MachineFunction *MF = MBB->getParent();
  MachineBasicBlock *TrueMBB = MF->CreateMachineBasicBlock();
  MachineBasicBlock *SinkMBB = MF->CreateMachineBasicBlock();
  
  MachineFunction::iterator It = ++MBB->getIterator();
  MF->insert(It, TrueMBB);
  MF->insert(It, SinkMBB);
  
  // Transfer successors from MBB to SinkMBB
  SinkMBB->splice(SinkMBB->begin(), MBB, std::next(MI.getIterator()), MBB->end());
  SinkMBB->transferSuccessorsAndUpdatePHIs(MBB);
  
  // Set up edges
  MBB->addSuccessor(TrueMBB);
  MBB->addSuccessor(SinkMBB);
  TrueMBB->addSuccessor(SinkMBB);
  
  // Get operands
  Register DstReg = MI.getOperand(0).getReg();
  Register TrueReg = MI.getOperand(1).getReg();
  Register FalseReg = MI.getOperand(2).getReg();
  int64_t CC = MI.getOperand(3).getImm();
  
  // Map condition code to branch opcode
  unsigned BrOpc;
  switch (CC) {
  case ISD::SETEQ:  BrOpc = M65832::BEQ; break;
  case ISD::SETNE:  BrOpc = M65832::BNE; break;
  case ISD::SETLT:  BrOpc = M65832::BMI; break;
  case ISD::SETGE:  BrOpc = M65832::BPL; break;
  case ISD::SETULT: BrOpc = M65832::BCC; break;
  case ISD::SETUGE: BrOpc = M65832::BCS; break;
  case ISD::SETGT:  BrOpc = M65832::BNE; break;
  case ISD::SETLE:  BrOpc = M65832::BEQ; break;
  case ISD::SETUGT: BrOpc = M65832::BNE; break;
  case ISD::SETULE: BrOpc = M65832::BEQ; break;
  default:          BrOpc = M65832::BNE; break;
  }
  
  // MBB: Branch to TrueMBB if condition is true, else fall through to SinkMBB
  BuildMI(MBB, DL, TII.get(BrOpc)).addMBB(TrueMBB);
  BuildMI(MBB, DL, TII.get(M65832::BRA)).addMBB(SinkMBB);
  
  // TrueMBB: Empty, just used for PHI
  
  // SinkMBB: Create PHI node
  BuildMI(*SinkMBB, SinkMBB->begin(), DL, TII.get(M65832::PHI), DstReg)
      .addReg(FalseReg).addMBB(MBB)
      .addReg(TrueReg).addMBB(TrueMBB);
  
  MI.eraseFromParent();
  return SinkMBB;
}

MachineBasicBlock *M65832TargetLowering::emitSelectCC(MachineInstr &MI,
                                                        MachineBasicBlock *MBB) const {
  // SELECT_CC_PSEUDO: (dst, lhs, rhs, trueVal, falseVal, cc)
  // Compares lhs vs rhs, selects trueVal if cc is true, else falseVal
  const TargetInstrInfo &TII = *Subtarget.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  
  bool IsFP = (MI.getOpcode() == M65832::SELECT_CC_F32_PSEUDO || 
               MI.getOpcode() == M65832::SELECT_CC_F64_PSEUDO);
  
  MachineFunction *MF = MBB->getParent();
  MachineBasicBlock *TrueMBB = MF->CreateMachineBasicBlock();
  MachineBasicBlock *SinkMBB = MF->CreateMachineBasicBlock();
  
  MachineFunction::iterator It = ++MBB->getIterator();
  MF->insert(It, TrueMBB);
  MF->insert(It, SinkMBB);
  
  SinkMBB->splice(SinkMBB->begin(), MBB, std::next(MI.getIterator()), MBB->end());
  SinkMBB->transferSuccessorsAndUpdatePHIs(MBB);
  
  MBB->addSuccessor(TrueMBB);
  MBB->addSuccessor(SinkMBB);
  TrueMBB->addSuccessor(SinkMBB);
  
  Register DstReg = MI.getOperand(0).getReg();
  Register LHSReg = MI.getOperand(1).getReg();
  Register RHSReg = MI.getOperand(2).getReg();
  Register TrueReg = MI.getOperand(3).getReg();
  Register FalseReg = MI.getOperand(4).getReg();
  int64_t CC = MI.getOperand(5).getImm();

  // Use fused compare-and-branch terminator to prevent flag clobbering
  BuildMI(MBB, DL, TII.get(M65832::CMP_BR_CC))
      .addReg(LHSReg)
      .addReg(RHSReg)
      .addImm(CC)
      .addMBB(TrueMBB);
  BuildMI(MBB, DL, TII.get(M65832::BRA)).addMBB(SinkMBB);
  
  // TrueMBB: Empty (just a branch target), falls through to SinkMBB
  
  // SinkMBB: PHI to select the result
  const TargetRegisterClass *RC = IsFP ? 
    (MI.getOpcode() == M65832::SELECT_CC_F64_PSEUDO ? &M65832::FPR64RegClass : &M65832::FPR32RegClass) :
    &M65832::GPRRegClass;
  
  BuildMI(*SinkMBB, SinkMBB->begin(), DL, TII.get(M65832::PHI), DstReg)
      .addReg(FalseReg).addMBB(MBB)
      .addReg(TrueReg).addMBB(TrueMBB);
  
  MI.eraseFromParent();
  return SinkMBB;
}

MachineBasicBlock *M65832TargetLowering::emitSelectCCFP(MachineInstr &MI,
                                                          MachineBasicBlock *MBB) const {
  // SELECT_CC_FP_PSEUDO: (dst, trueVal, falseVal, cc)
  // Uses flags already set by FCMP (no comparison in this pseudo)
  const TargetInstrInfo &TII = *Subtarget.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  
  MachineFunction *MF = MBB->getParent();
  MachineBasicBlock *TrueMBB = MF->CreateMachineBasicBlock();
  MachineBasicBlock *SinkMBB = MF->CreateMachineBasicBlock();
  
  MachineFunction::iterator It = ++MBB->getIterator();
  MF->insert(It, TrueMBB);
  MF->insert(It, SinkMBB);
  
  SinkMBB->splice(SinkMBB->begin(), MBB, std::next(MI.getIterator()), MBB->end());
  SinkMBB->transferSuccessorsAndUpdatePHIs(MBB);
  
  MBB->addSuccessor(TrueMBB);
  MBB->addSuccessor(SinkMBB);
  TrueMBB->addSuccessor(SinkMBB);
  
  // Get operands: dst, trueVal, falseVal, cc
  Register DstReg = MI.getOperand(0).getReg();
  Register TrueReg = MI.getOperand(1).getReg();
  Register FalseReg = MI.getOperand(2).getReg();
  int64_t CC = MI.getOperand(3).getImm();
  
  // Map condition code to branch opcode (flags already set by FCMP)
  unsigned BrOpc;
  switch (CC) {
  case ISD::SETEQ:  BrOpc = M65832::BEQ; break;
  case ISD::SETNE:  BrOpc = M65832::BNE; break;
  case ISD::SETLT:  BrOpc = M65832::BMI; break;
  case ISD::SETGE:  BrOpc = M65832::BPL; break;
  case ISD::SETULT: BrOpc = M65832::BCC; break;
  case ISD::SETUGE: BrOpc = M65832::BCS; break;
  case ISD::SETGT:  BrOpc = M65832::BNE; break;
  case ISD::SETLE:  BrOpc = M65832::BEQ; break;
  case ISD::SETUGT: BrOpc = M65832::BNE; break;
  case ISD::SETULE: BrOpc = M65832::BEQ; break;
  default:          BrOpc = M65832::BNE; break;
  }
  
  // MBB: Branch to TrueMBB if condition is true, else fall through
  BuildMI(MBB, DL, TII.get(BrOpc)).addMBB(TrueMBB);
  BuildMI(MBB, DL, TII.get(M65832::BRA)).addMBB(SinkMBB);
  
  // TrueMBB: Empty, just used for PHI
  
  // SinkMBB: Create PHI node
  BuildMI(*SinkMBB, SinkMBB->begin(), DL, TII.get(M65832::PHI), DstReg)
      .addReg(FalseReg).addMBB(MBB)
      .addReg(TrueReg).addMBB(TrueMBB);
  
  MI.eraseFromParent();
  return SinkMBB;
}

//===----------------------------------------------------------------------===//
//                         Inline Assembly Support
//===----------------------------------------------------------------------===//

TargetLowering::ConstraintType
M65832TargetLowering::getConstraintType(StringRef Constraint) const {
  if (Constraint.size() == 1) {
    switch (Constraint[0]) {
    default:
      break;
    case 'r':  // General-purpose register
    case 'a':  // Accumulator A
    case 'x':  // X index register
    case 'y':  // Y index register
    case 'f':  // FPU register
      return C_RegisterClass;
    case 'm':  // Memory operand
    case 'o':  // Memory operand with offsettable addressing
      return C_Memory;
    }
  }

  // Explicit register constraints: {R0}, {A}, etc.
  if (Constraint.size() > 2 && Constraint.front() == '{' &&
      Constraint.back() == '}') {
    return C_Register;
  }

  return TargetLowering::getConstraintType(Constraint);
}

std::pair<unsigned, const TargetRegisterClass *>
M65832TargetLowering::getRegForInlineAsmConstraint(
    const TargetRegisterInfo *TRI, StringRef Constraint, MVT VT) const {
  if (Constraint.size() == 1) {
    switch (Constraint[0]) {
    default:
      break;
    case 'r':
      // General-purpose register
      if (VT == MVT::i32 || VT == MVT::i16 || VT == MVT::i8)
        return std::make_pair(0U, &M65832::GPRRegClass);
      break;
    case 'a':
      // Accumulator A
      if (VT == MVT::i32 || VT == MVT::i16 || VT == MVT::i8)
        return std::make_pair(M65832::A, &M65832::ACCRegClass);
      break;
    case 'x':
      // X index register
      if (VT == MVT::i32 || VT == MVT::i16 || VT == MVT::i8)
        return std::make_pair(M65832::X, &M65832::XREGRegClass);
      break;
    case 'y':
      // Y index register
      if (VT == MVT::i32 || VT == MVT::i16 || VT == MVT::i8)
        return std::make_pair(M65832::Y, &M65832::YREGRegClass);
      break;
    case 'f':
      // FPU register
      if (VT == MVT::f64)
        return std::make_pair(0U, &M65832::FPR64RegClass);
      if (VT == MVT::f32)
        return std::make_pair(0U, &M65832::FPR32RegClass);
      break;
    }
  }

  // Handle explicit register names: {R0}, {R1}, ..., {R63}, {A}, {X}, {Y}, etc.
  if (Constraint.size() > 2 && Constraint.front() == '{' &&
      Constraint.back() == '}') {
    StringRef RegName = Constraint.slice(1, Constraint.size() - 1);

    // GPR registers R0-R63 (case-insensitive)
    if (RegName.size() >= 2 &&
        (RegName[0] == 'R' || RegName[0] == 'r')) {
      unsigned RegNum;
      if (!RegName.substr(1).getAsInteger(10, RegNum) && RegNum <= 63) {
        static const unsigned GPRRegs[] = {
          M65832::R0,  M65832::R1,  M65832::R2,  M65832::R3,
          M65832::R4,  M65832::R5,  M65832::R6,  M65832::R7,
          M65832::R8,  M65832::R9,  M65832::R10, M65832::R11,
          M65832::R12, M65832::R13, M65832::R14, M65832::R15,
          M65832::R16, M65832::R17, M65832::R18, M65832::R19,
          M65832::R20, M65832::R21, M65832::R22, M65832::R23,
          M65832::R24, M65832::R25, M65832::R26, M65832::R27,
          M65832::R28, M65832::R29, M65832::R30, M65832::R31,
          M65832::R32, M65832::R33, M65832::R34, M65832::R35,
          M65832::R36, M65832::R37, M65832::R38, M65832::R39,
          M65832::R40, M65832::R41, M65832::R42, M65832::R43,
          M65832::R44, M65832::R45, M65832::R46, M65832::R47,
          M65832::R48, M65832::R49, M65832::R50, M65832::R51,
          M65832::R52, M65832::R53, M65832::R54, M65832::R55,
          M65832::R56, M65832::R57, M65832::R58, M65832::R59,
          M65832::R60, M65832::R61, M65832::R62, M65832::R63
        };
        return std::make_pair(GPRRegs[RegNum], &M65832::GPRRegClass);
      }
    }

    // FPU registers F0-F15 (case-insensitive)
    if (RegName.size() >= 2 &&
        (RegName[0] == 'F' || RegName[0] == 'f')) {
      unsigned RegNum;
      if (!RegName.substr(1).getAsInteger(10, RegNum) && RegNum <= 15) {
        static const unsigned FPRRegs[] = {
          M65832::F0,  M65832::F1,  M65832::F2,  M65832::F3,
          M65832::F4,  M65832::F5,  M65832::F6,  M65832::F7,
          M65832::F8,  M65832::F9,  M65832::F10, M65832::F11,
          M65832::F12, M65832::F13, M65832::F14, M65832::F15
        };
        if (VT == MVT::f64)
          return std::make_pair(FPRRegs[RegNum], &M65832::FPR64RegClass);
        return std::make_pair(FPRRegs[RegNum], &M65832::FPR32RegClass);
      }
    }

    // Architectural registers (case-insensitive)
    if (RegName.equals_insensitive("a"))
      return std::make_pair(M65832::A, &M65832::ACCRegClass);
    if (RegName.equals_insensitive("x"))
      return std::make_pair(M65832::X, &M65832::IDXREGRegClass);
    if (RegName.equals_insensitive("y"))
      return std::make_pair(M65832::Y, &M65832::IDXREGRegClass);
    if (RegName.equals_insensitive("sp"))
      return std::make_pair(M65832::SP, &M65832::SPREGRegClass);
    if (RegName.equals_insensitive("t"))
      return std::make_pair(M65832::T, &M65832::TREGRegClass);

    // Also handle aliases: gp, fp, lr
    if (RegName.equals_insensitive("gp"))
      return std::make_pair(M65832::R28, &M65832::GPRRegClass);
    if (RegName.equals_insensitive("fp"))
      return std::make_pair(M65832::R29, &M65832::GPRRegClass);
    if (RegName.equals_insensitive("lr"))
      return std::make_pair(M65832::R30, &M65832::GPRRegClass);
  }

  return TargetLowering::getRegForInlineAsmConstraint(TRI, Constraint, VT);
}
