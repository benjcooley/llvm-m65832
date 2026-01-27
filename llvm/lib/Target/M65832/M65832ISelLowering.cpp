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
  setOperationAction(ISD::SELECT_CC, MVT::f32, Expand);
  setOperationAction(ISD::SELECT_CC, MVT::f64, Expand);
  setOperationAction(ISD::SELECT, MVT::f32, Expand);
  setOperationAction(ISD::SELECT, MVT::f64, Expand);
  
  // Floating point constants - always expand (load from constant pool)
  setOperationAction(ISD::ConstantFP, MVT::f32, Expand);
  setOperationAction(ISD::ConstantFP, MVT::f64, Expand);
  
  // Bitcast between float and int
  setOperationAction(ISD::BITCAST, MVT::f32, Legal);
  setOperationAction(ISD::BITCAST, MVT::i32, Legal);
  setOperationAction(ISD::BITCAST, MVT::f64, Expand);
  setOperationAction(ISD::BITCAST, MVT::i64, Expand);
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
  case M65832ISD::SELECT_CC:    return "M65832ISD::SELECT_CC";
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
  
  // Generate compare - use FCMP for float types, CMP for integers
  SDValue Cmp;
  EVT CmpVT = LHS.getValueType();
  if (CmpVT == MVT::f32 || CmpVT == MVT::f64) {
    Cmp = DAG.getNode(M65832ISD::FCMP, DL, MVT::Glue, LHS, RHS);
  } else {
    Cmp = DAG.getNode(M65832ISD::CMP, DL, MVT::Glue, LHS, RHS);
  }
  
  // Generate branch
  SDValue CCVal = DAG.getConstant(CC, DL, MVT::i32);
  return DAG.getNode(M65832ISD::BR_CC, DL, Op.getValueType(), Chain, Dest,
                     CCVal, Cmp);
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
  
  // For integers, include LHS/RHS so each SELECT has its own CMP
  // This ensures flags aren't clobbered by intervening instructions
  SDValue CCVal = DAG.getConstant(CC, DL, MVT::i32);
  return DAG.getNode(M65832ISD::SELECT_CC, DL, Op.getValueType(), 
                     LHS, RHS, TrueVal, FalseVal, CCVal);
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
  
  // For integers, include LHS/RHS for comparison
  return DAG.getNode(M65832ISD::SELECT_CC, DL, MVT::i32, 
                     LHS, RHS, One, Zero, CCVal);
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
  
  // Adjust stack
  Chain = DAG.getCALLSEQ_START(Chain, StackSize, 0, DL);
  
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
  Chain = DAG.getCALLSEQ_END(Chain, StackSize, 0, InGlue, DL);
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
    return emitSelect(MI, MBB);
  default:
    llvm_unreachable("Unexpected instruction for custom inserter");
  }
}

MachineBasicBlock *M65832TargetLowering::emitSelect(MachineInstr &MI,
                                                      MachineBasicBlock *MBB) const {
  // This implements the SELECT pseudo by expanding to branches
  // Strategy: branch on condition, copy appropriate value to destination
  const TargetInstrInfo &TII = *Subtarget.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  
  // Create new basic blocks
  MachineFunction *MF = MBB->getParent();
  MachineBasicBlock *CopyMBB = MF->CreateMachineBasicBlock();
  MachineBasicBlock *SinkMBB = MF->CreateMachineBasicBlock();
  
  MachineFunction::iterator It = ++MBB->getIterator();
  MF->insert(It, CopyMBB);
  MF->insert(It, SinkMBB);
  
  // Transfer successors from MBB to SinkMBB
  SinkMBB->splice(SinkMBB->begin(), MBB, std::next(MI.getIterator()), MBB->end());
  SinkMBB->transferSuccessorsAndUpdatePHIs(MBB);
  
  // Set up edges
  MBB->addSuccessor(CopyMBB);
  MBB->addSuccessor(SinkMBB);
  CopyMBB->addSuccessor(SinkMBB);
  
  // Get operands
  Register DstReg = MI.getOperand(0).getReg();
  Register TrueReg = MI.getOperand(1).getReg();
  Register FalseReg = MI.getOperand(2).getReg();
  int64_t CC = MI.getOperand(3).getImm();
  
  // Convert registers to DP offsets (R0=$00, R1=$04, ...)
  auto getDPOffset = [](Register Reg) -> unsigned {
    unsigned RegNum = Reg - M65832::R0;
    return RegNum * 4;
  };
  
  unsigned DstDP = getDPOffset(DstReg);
  unsigned TrueDP = getDPOffset(TrueReg);
  unsigned FalseDP = getDPOffset(FalseReg);
  
  // Map condition code to branch opcode
  // We branch to CopyMBB when condition is TRUE to copy TrueReg
  // We fall through to SinkMBB when FALSE (after copying FalseReg first)
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
  
  // In MBB: First copy false value to dest, then conditionally branch
  // This way if condition is false, we already have the right value
  BuildMI(MBB, DL, TII.get(M65832::LDA_DP), M65832::A)
      .addImm(FalseDP);
  BuildMI(MBB, DL, TII.get(M65832::STA_DP))
      .addReg(M65832::A, RegState::Kill)
      .addImm(DstDP);
  BuildMI(MBB, DL, TII.get(BrOpc)).addMBB(CopyMBB);
  // Fall through to SinkMBB
  
  // CopyMBB: Copy true value (overwrites false value)
  BuildMI(CopyMBB, DL, TII.get(M65832::LDA_DP), M65832::A)
      .addImm(TrueDP);
  BuildMI(CopyMBB, DL, TII.get(M65832::STA_DP))
      .addReg(M65832::A, RegState::Kill)
      .addImm(DstDP);
  // Fall through to SinkMBB
  
  MI.eraseFromParent();
  return SinkMBB;
}
