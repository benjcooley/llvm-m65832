//===-- M65832AsmParser.cpp - Parse M65832 assembly to MCInst ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/M65832MCTargetDesc.h"
#include "TargetInfo/M65832TargetInfo.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Casting.h"

using namespace llvm;

#define DEBUG_TYPE "m65832-asm-parser"

namespace {

/// M65832Operand - Instances of this class represent a parsed operand.
class M65832Operand : public MCParsedAsmOperand {
public:
  enum KindTy {
    k_Token,
    k_Immediate,
    k_Register,
    k_Memory
  };

private:
  KindTy Kind;
  SMLoc StartLoc, EndLoc;

  struct TokOp {
    const char *Data;
    unsigned Length;
  };

  struct RegOp {
    unsigned RegNum;
  };

  struct ImmOp {
    const MCExpr *Val;
  };

  struct MemOp {
    unsigned BaseReg;
    const MCExpr *Disp;
    unsigned IndexReg;
    bool Indirect;
    bool IndirectLong;
    bool StackRelative;
  };

  union {
    TokOp Tok;
    RegOp Reg;
    ImmOp Imm;
    MemOp Mem;
  };

public:
  M65832Operand(KindTy K, SMLoc S, SMLoc E) : Kind(K), StartLoc(S), EndLoc(E) {}

  // Token operand
  static std::unique_ptr<M65832Operand> createToken(StringRef Str, SMLoc S) {
    auto Op = std::make_unique<M65832Operand>(k_Token, S, S);
    Op->Tok.Data = Str.data();
    Op->Tok.Length = Str.size();
    return Op;
  }

  // Immediate operand
  static std::unique_ptr<M65832Operand> createImm(const MCExpr *Val, SMLoc S,
                                                   SMLoc E) {
    auto Op = std::make_unique<M65832Operand>(k_Immediate, S, E);
    Op->Imm.Val = Val;
    return Op;
  }

  // Register operand
  static std::unique_ptr<M65832Operand> createReg(unsigned RegNo, SMLoc S,
                                                   SMLoc E) {
    auto Op = std::make_unique<M65832Operand>(k_Register, S, E);
    Op->Reg.RegNum = RegNo;
    return Op;
  }

  // Memory operand
  static std::unique_ptr<M65832Operand>
  createMem(unsigned Base, const MCExpr *Disp, unsigned Index, bool Indirect,
            bool IndirectLong, bool StackRel, SMLoc S, SMLoc E) {
    auto Op = std::make_unique<M65832Operand>(k_Memory, S, E);
    Op->Mem.BaseReg = Base;
    Op->Mem.Disp = Disp;
    Op->Mem.IndexReg = Index;
    Op->Mem.Indirect = Indirect;
    Op->Mem.IndirectLong = IndirectLong;
    Op->Mem.StackRelative = StackRel;
    return Op;
  }

  SMLoc getStartLoc() const override { return StartLoc; }
  SMLoc getEndLoc() const override { return EndLoc; }

  bool isToken() const override { return Kind == k_Token; }
  bool isImm() const override { return Kind == k_Immediate; }
  bool isReg() const override { return Kind == k_Register; }
  bool isMem() const override { return Kind == k_Memory; }

  StringRef getToken() const {
    assert(Kind == k_Token && "Not a token");
    return StringRef(Tok.Data, Tok.Length);
  }

  MCRegister getReg() const override {
    assert(Kind == k_Register && "Not a register");
    return Reg.RegNum;
  }

  const MCExpr *getImm() const {
    assert(Kind == k_Immediate && "Not an immediate");
    return Imm.Val;
  }

  void print(raw_ostream &OS, const MCAsmInfo &MAI) const override {
    switch (Kind) {
    case k_Token:
      OS << "'" << getToken() << "'";
      break;
    case k_Immediate:
      OS << "<imm: ";
      MAI.printExpr(OS, *Imm.Val);
      OS << ">";
      break;
    case k_Register:
      OS << "<reg: " << Reg.RegNum << ">";
      break;
    case k_Memory:
      OS << "<mem>";
      break;
    }
  }

  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::createReg(getReg()));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::createExpr(getImm()));
  }
};

class M65832AsmParser : public MCTargetAsmParser {
  const MCInstrInfo &MII;

  unsigned parseRegisterName(StringRef Name);
  bool parseExpression(const MCExpr *&Res, SMLoc &EndLoc);

  ParseStatus parseOperand(OperandVector &Operands, StringRef Mnemonic);
  ParseStatus parseImmediate(OperandVector &Operands);
  ParseStatus parseMemoryOperand(OperandVector &Operands);

  unsigned validateTargetOperandClass(MCParsedAsmOperand &Op,
                                      unsigned Kind) override;

#define GET_ASSEMBLER_HEADER
#include "M65832GenAsmMatcher.inc"

public:
  M65832AsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
                  const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, STI, MII), MII(MII) {
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
  }

  bool parseRegister(MCRegister &Reg, SMLoc &StartLoc, SMLoc &EndLoc) override;
  ParseStatus tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                               SMLoc &EndLoc) override;
  bool parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;
  bool matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;
  ParseStatus parseDirective(AsmToken DirectiveID) override;
};

} // end anonymous namespace

unsigned M65832AsmParser::parseRegisterName(StringRef Name) {
  // GPR registers R0-R15
  if (Name.size() >= 2 && (Name[0] == 'R' || Name[0] == 'r')) {
    unsigned RegNum;
    if (!Name.substr(1).getAsInteger(10, RegNum) && RegNum <= 15) {
      switch (RegNum) {
      case 0: return M65832::R0;
      case 1: return M65832::R1;
      case 2: return M65832::R2;
      case 3: return M65832::R3;
      case 4: return M65832::R4;
      case 5: return M65832::R5;
      case 6: return M65832::R6;
      case 7: return M65832::R7;
      case 8: return M65832::R8;
      case 9: return M65832::R9;
      case 10: return M65832::R10;
      case 11: return M65832::R11;
      case 12: return M65832::R12;
      case 13: return M65832::R13;
      case 14: return M65832::R14;
      case 15: return M65832::R15;
      default: return 0;
      }
    }
  }

  // FPU registers F0-F15
  if (Name.size() >= 2 && (Name[0] == 'F' || Name[0] == 'f')) {
    unsigned RegNum;
    if (!Name.substr(1).getAsInteger(10, RegNum) && RegNum <= 15) {
      switch (RegNum) {
      case 0: return M65832::F0;
      case 1: return M65832::F1;
      case 2: return M65832::F2;
      case 3: return M65832::F3;
      case 4: return M65832::F4;
      case 5: return M65832::F5;
      case 6: return M65832::F6;
      case 7: return M65832::F7;
      case 8: return M65832::F8;
      case 9: return M65832::F9;
      case 10: return M65832::F10;
      case 11: return M65832::F11;
      case 12: return M65832::F12;
      case 13: return M65832::F13;
      case 14: return M65832::F14;
      case 15: return M65832::F15;
      default: return 0;
      }
    }
  }

  // Special registers
  return StringSwitch<unsigned>(Name.upper())
      .Case("A", M65832::A)
      .Case("X", M65832::X)
      .Case("Y", M65832::Y)
      .Case("SP", M65832::SP)
      .Default(0);
}

bool M65832AsmParser::parseRegister(MCRegister &Reg, SMLoc &StartLoc,
                                     SMLoc &EndLoc) {
  return tryParseRegister(Reg, StartLoc, EndLoc).isFailure();
}

ParseStatus M65832AsmParser::tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                                               SMLoc &EndLoc) {
  const AsmToken &Tok = getParser().getTok();
  StartLoc = Tok.getLoc();

  if (Tok.isNot(AsmToken::Identifier))
    return ParseStatus::NoMatch;

  unsigned RegNo = parseRegisterName(Tok.getString());
  if (RegNo == 0)
    return ParseStatus::NoMatch;

  Reg = RegNo;
  EndLoc = Tok.getEndLoc();
  getParser().Lex();
  return ParseStatus::Success;
}

bool M65832AsmParser::parseExpression(const MCExpr *&Res, SMLoc &EndLoc) {
  return getParser().parseExpression(Res, EndLoc);
}

ParseStatus M65832AsmParser::parseImmediate(OperandVector &Operands) {
  SMLoc S = getParser().getTok().getLoc();
  SMLoc E;

  if (!getParser().getTok().is(AsmToken::Hash))
    return ParseStatus::NoMatch;

  getParser().Lex(); // Eat '#'

  const MCExpr *Expr;
  if (parseExpression(Expr, E))
    return ParseStatus::Failure;

  Operands.push_back(M65832Operand::createImm(Expr, S, E));
  return ParseStatus::Success;
}

ParseStatus M65832AsmParser::parseMemoryOperand(OperandVector &Operands) {
  SMLoc S = getParser().getTok().getLoc();
  SMLoc E;
  const MCExpr *Disp = nullptr;
  unsigned BaseReg = 0;
  unsigned IndexReg = 0;
  bool Indirect = false;
  bool IndirectLong = false;
  bool StackRelative = false;

  // Check for indirect: (addr) or [addr]
  if (getParser().getTok().is(AsmToken::LParen)) {
    Indirect = true;
    getParser().Lex();
  } else if (getParser().getTok().is(AsmToken::LBrac)) {
    IndirectLong = true;
    getParser().Lex();
  }

  // Parse displacement
  if (parseExpression(Disp, E))
    return ParseStatus::Failure;

  // Check for ,X ,Y ,S
  if (getParser().getTok().is(AsmToken::Comma)) {
    getParser().Lex();
    StringRef IndexName = getParser().getTok().getString();
    if (IndexName.equals_insensitive("X")) {
      IndexReg = M65832::X;
      getParser().Lex();
    } else if (IndexName.equals_insensitive("Y")) {
      IndexReg = M65832::Y;
      getParser().Lex();
    } else if (IndexName.equals_insensitive("S")) {
      StackRelative = true;
      getParser().Lex();
    }
  }

  // Close bracket
  if (Indirect) {
    if (!getParser().getTok().is(AsmToken::RParen))
      return Error(getParser().getTok().getLoc(), "expected ')'");
    getParser().Lex();
    // Check for ),Y
    if (getParser().getTok().is(AsmToken::Comma)) {
      getParser().Lex();
      if (getParser().getTok().getString().equals_insensitive("Y")) {
        IndexReg = M65832::Y;
        getParser().Lex();
      }
    }
  } else if (IndirectLong) {
    if (!getParser().getTok().is(AsmToken::RBrac))
      return Error(getParser().getTok().getLoc(), "expected ']'");
    getParser().Lex();
    // Check for ],Y
    if (getParser().getTok().is(AsmToken::Comma)) {
      getParser().Lex();
      if (getParser().getTok().getString().equals_insensitive("Y")) {
        IndexReg = M65832::Y;
        getParser().Lex();
      }
    }
  }

  E = getParser().getTok().getLoc();
  Operands.push_back(M65832Operand::createMem(BaseReg, Disp, IndexReg, Indirect,
                                               IndirectLong, StackRelative, S, E));
  return ParseStatus::Success;
}

ParseStatus M65832AsmParser::parseOperand(OperandVector &Operands,
                                           StringRef Mnemonic) {
  // Try immediate
  if (getParser().getTok().is(AsmToken::Hash))
    return parseImmediate(Operands);

  // Try register
  if (getParser().getTok().is(AsmToken::Identifier)) {
    unsigned RegNo = parseRegisterName(getParser().getTok().getString());
    if (RegNo != 0) {
      SMLoc S = getParser().getTok().getLoc();
      SMLoc E = getParser().getTok().getEndLoc();
      Operands.push_back(M65832Operand::createReg(RegNo, S, E));
      getParser().Lex();
      return ParseStatus::Success;
    }
  }

  // Try memory operand
  return parseMemoryOperand(Operands);
}

bool M65832AsmParser::parseInstruction(ParseInstructionInfo &Info,
                                        StringRef Name, SMLoc NameLoc,
                                        OperandVector &Operands) {
  // Add mnemonic as first operand
  Operands.push_back(M65832Operand::createToken(Name, NameLoc));

  // If no more tokens, implied addressing
  if (getParser().getTok().is(AsmToken::EndOfStatement))
    return false;

  // Parse first operand
  if (!parseOperand(Operands, Name).isSuccess())
    return true;

  // Parse additional comma-separated operands
  while (getParser().getTok().is(AsmToken::Comma)) {
    getParser().Lex();
    if (!parseOperand(Operands, Name).isSuccess())
      return true;
  }

  if (!getParser().getTok().is(AsmToken::EndOfStatement))
    return Error(getParser().getTok().getLoc(), "unexpected token in operand");

  return false;
}

ParseStatus M65832AsmParser::parseDirective(AsmToken DirectiveID) {
  StringRef IDVal = DirectiveID.getIdentifier();

  if (IDVal == ".m8" || IDVal == ".m16" || IDVal == ".m32")
    return ParseStatus::Success;

  return ParseStatus::NoMatch;
}

bool M65832AsmParser::matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                               OperandVector &Operands,
                                               MCStreamer &Out,
                                               uint64_t &ErrorInfo,
                                               bool MatchingInlineAsm) {
  MCInst Inst;

  switch (MatchInstructionImpl(Operands, Inst, ErrorInfo, MatchingInlineAsm)) {
  case Match_Success:
    Out.emitInstruction(Inst, getSTI());
    return false;
  case Match_MissingFeature:
    return Error(IDLoc, "instruction requires a CPU feature not enabled");
  case Match_InvalidOperand: {
    SMLoc ErrorLoc = IDLoc;
    if (ErrorInfo != ~0ULL && ErrorInfo < Operands.size())
      ErrorLoc = ((M65832Operand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(ErrorLoc, "invalid operand for instruction");
  }
  case Match_MnemonicFail:
    return Error(IDLoc, "unrecognized instruction mnemonic");
  default:
    return true;
  }
}

unsigned M65832AsmParser::validateTargetOperandClass(MCParsedAsmOperand &AsmOp,
                                                      unsigned Kind) {
  return Match_InvalidOperand;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeM65832AsmParser() {
  RegisterMCAsmParser<M65832AsmParser> X(getTheM65832Target());
}

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "M65832GenAsmMatcher.inc"
