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
    bool BRelative;  // B+offset syntax (B is frame pointer)
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
            bool IndirectLong, bool StackRel, bool BRel, SMLoc S, SMLoc E) {
    auto Op = std::make_unique<M65832Operand>(k_Memory, S, E);
    Op->Mem.BaseReg = Base;
    Op->Mem.Disp = Disp;
    Op->Mem.IndexReg = Index;
    Op->Mem.Indirect = Indirect;
    Op->Mem.IndirectLong = IndirectLong;
    Op->Mem.StackRelative = StackRel;
    Op->Mem.BRelative = BRel;
    return Op;
  }

  SMLoc getStartLoc() const override { return StartLoc; }
  SMLoc getEndLoc() const override { return EndLoc; }

  bool isToken() const override { return Kind == k_Token; }
  bool isImm() const override { return Kind == k_Immediate; }
  bool isReg() const override { return Kind == k_Register; }
  bool isMem() const override { return Kind == k_Memory; }

  // AsmMatcher operand type predicates
  bool isM65832Imm() const { return Kind == k_Immediate; }
  
  // Check if this is a GPR register (R0-R63)
  bool isGPRReg() const {
    if (Kind != k_Register)
      return false;
    unsigned RegNo = Reg.RegNum;
    return RegNo >= M65832::R0 && RegNo <= M65832::R63;
  }
  
  // Check if this is indirect register addressing (Rn) - memory with BaseReg set
  bool isIndirectReg() const {
    if (Kind != k_Memory)
      return false;
    // Must have BaseReg set, be indirect, no Y index
    return Mem.BaseReg != 0 && Mem.Indirect && Mem.IndexReg == 0;
  }
  
  // Check if this is indirect Y-indexed addressing (Rn),Y
  bool isIndirectRegY() const {
    if (Kind != k_Memory)
      return false;
    // Must have BaseReg set, be indirect, with Y index
    return Mem.BaseReg != 0 && Mem.Indirect && Mem.IndexReg == M65832::Y;
  }

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
    if (isImm())
      Inst.addOperand(MCOperand::createExpr(getImm()));
    else if (isMem())
      Inst.addOperand(MCOperand::createExpr(Mem.Disp));
  }

  void addMemOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    // Memory operand - if we have a base register (indirect register addressing),
    // add it as a register operand; otherwise add displacement as expression
    if (Mem.BaseReg != 0) {
      Inst.addOperand(MCOperand::createReg(Mem.BaseReg));
    } else if (Mem.Disp) {
      Inst.addOperand(MCOperand::createExpr(Mem.Disp));
    } else {
      // Fallback - should not happen
      Inst.addOperand(MCOperand::createImm(0));
    }
  }
};

class M65832AsmParser : public MCTargetAsmParser {
  const MCInstrInfo &MII;

  unsigned parseRegisterName(StringRef Name);
  bool parseHexNumber(int64_t &Value);
  bool parseM65832Expression(const MCExpr *&Res, SMLoc &EndLoc);

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
  // GPR registers R0-R63
  if (Name.size() >= 2 && (Name[0] == 'R' || Name[0] == 'r')) {
    unsigned RegNum;
    if (!Name.substr(1).getAsInteger(10, RegNum) && RegNum <= 63) {
      // Map register number to LLVM register enum
      static const unsigned GPRRegs[] = {
        M65832::R0, M65832::R1, M65832::R2, M65832::R3,
        M65832::R4, M65832::R5, M65832::R6, M65832::R7,
        M65832::R8, M65832::R9, M65832::R10, M65832::R11,
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
      return GPRRegs[RegNum];
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

// Parse hex number with $ prefix (6502 style) or 0x prefix
// This is tricky because $01FF gets tokenized as $ + 01 + FF by the lexer
bool M65832AsmParser::parseHexNumber(int64_t &Value) {
  const AsmToken &Tok = getParser().getTok();
  
  if (Tok.is(AsmToken::Dollar)) {
    // $hex syntax - consume adjacent tokens that form hex digits
    SMLoc PrevEnd = Tok.getEndLoc();
    getParser().Lex(); // Eat '$'
    
    std::string HexStr;
    
    // Keep consuming tokens that are adjacent and look like hex
    while (true) {
      const AsmToken &NextTok = getParser().getTok();
      
      // Check if this token is adjacent to the previous one
      if (NextTok.getLoc().getPointer() != PrevEnd.getPointer())
        break;
      
      StringRef TokStr;
      
      if (NextTok.is(AsmToken::Integer)) {
        TokStr = NextTok.getString();
      } else if (NextTok.is(AsmToken::Identifier)) {
        TokStr = NextTok.getString();
        // Check if it's all hex digits
        bool AllHex = true;
        for (char c : TokStr) {
          if (!isxdigit(c)) {
            AllHex = false;
            break;
          }
        }
        if (!AllHex)
          break;
      } else {
        break;
      }
      
      HexStr += TokStr.str();
      PrevEnd = NextTok.getEndLoc();
      getParser().Lex();
    }
    
    if (HexStr.empty())
      return Error(getParser().getTok().getLoc(), "expected hex number after '$'");
    
    // Parse as hex
    if (StringRef(HexStr).getAsInteger(16, Value))
      return Error(getParser().getTok().getLoc(), "invalid hex number");
    
    return false;
  }
  
  if (Tok.is(AsmToken::Integer)) {
    Value = Tok.getIntVal();
    getParser().Lex();
    return false;
  }
  
  return true;
}

// Parse expression with support for $hex syntax
bool M65832AsmParser::parseM65832Expression(const MCExpr *&Res, SMLoc &EndLoc) {
  const AsmToken &Tok = getParser().getTok();
  
  // Handle $hex prefix
  if (Tok.is(AsmToken::Dollar)) {
    SMLoc StartLoc = Tok.getLoc();
    int64_t Value;
    if (parseHexNumber(Value))
      return true;
    Res = MCConstantExpr::create(Value, getContext());
    EndLoc = getParser().getTok().getLoc();
    return false;
  }
  
  // Handle B+symbol for B-relative addressing (B is frame pointer)
  if (Tok.is(AsmToken::Identifier) && 
      Tok.getString().equals_insensitive("B")) {
    getParser().Lex(); // Eat 'B'
    if (getParser().getTok().isNot(AsmToken::Plus))
      return Error(getParser().getTok().getLoc(), "expected '+' after B");
    getParser().Lex(); // Eat '+'
    // Parse the symbol/expression after B+
    return getParser().parseExpression(Res, EndLoc);
  }
  
  // Default to standard expression parsing
  return getParser().parseExpression(Res, EndLoc);
}

ParseStatus M65832AsmParser::parseImmediate(OperandVector &Operands) {
  SMLoc S = getParser().getTok().getLoc();
  SMLoc E;

  if (!getParser().getTok().is(AsmToken::Hash))
    return ParseStatus::NoMatch;

  // Add '#' as a token operand (the matcher expects it)
  Operands.push_back(M65832Operand::createToken("#", S));
  getParser().Lex(); // Eat '#'

  const MCExpr *Expr;
  if (parseM65832Expression(Expr, E))
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
  bool BRelative = false;

  // Check for B+offset (B-relative, B is frame pointer)
  if (getParser().getTok().is(AsmToken::Identifier) &&
      getParser().getTok().getString().equals_insensitive("B")) {
    BRelative = true;
    getParser().Lex(); // Eat 'B'
    if (getParser().getTok().is(AsmToken::Plus)) {
      getParser().Lex(); // Eat '+'
    }
  }

  // Check for indirect: (addr) or [addr]
  if (getParser().getTok().is(AsmToken::LParen)) {
    Indirect = true;
    getParser().Lex();
    
    // Check if the next token is a GPR register (for indirect register addressing)
    // In M65832 32-bit mode, (Rn) means indirect through register Rn (true register, not DP)
    if (getParser().getTok().is(AsmToken::Identifier)) {
      StringRef TokStr = getParser().getTok().getString();
      unsigned RegNo = parseRegisterName(TokStr);
      if (RegNo != 0 && RegNo >= M65832::R0 && RegNo <= M65832::R63) {
        // Indirect register addressing: (Rn) - store the register
        BaseReg = RegNo;
        getParser().Lex();  // Consume the register name
        goto close_paren;
      }
    }
  } else if (getParser().getTok().is(AsmToken::LBrac)) {
    IndirectLong = true;
    getParser().Lex();
    
    // Check if the next token is a GPR register
    if (getParser().getTok().is(AsmToken::Identifier)) {
      StringRef TokStr = getParser().getTok().getString();
      unsigned RegNo = parseRegisterName(TokStr);
      if (RegNo != 0 && RegNo >= M65832::R0 && RegNo <= M65832::R63) {
        // Indirect long register addressing: [Rn]
        BaseReg = RegNo;
        getParser().Lex();  // Consume the register name
        goto close_bracket;
      }
    }
  }

  // Parse displacement using M65832 expression parser
  if (parseM65832Expression(Disp, E))
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
close_paren:
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
close_bracket:
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
                                               IndirectLong, StackRelative, 
                                               BRelative, S, E));
  return ParseStatus::Success;
}

ParseStatus M65832AsmParser::parseOperand(OperandVector &Operands,
                                           StringRef Mnemonic) {
  // Try immediate (#value)
  if (getParser().getTok().is(AsmToken::Hash))
    return parseImmediate(Operands);

  // Try register
  if (getParser().getTok().is(AsmToken::Identifier)) {
    StringRef TokStr = getParser().getTok().getString();
    // Check if it's a register (but not 'B' which starts B-relative)
    if (!TokStr.equals_insensitive("B")) {
      unsigned RegNo = parseRegisterName(TokStr);
      if (RegNo != 0) {
        SMLoc S = getParser().getTok().getLoc();
        SMLoc E = getParser().getTok().getEndLoc();
        Operands.push_back(M65832Operand::createReg(RegNo, S, E));
        getParser().Lex();
        return ParseStatus::Success;
      }
    }
  }

  // Try memory operand (includes B+addr, $addr, symbol, etc.)
  return parseMemoryOperand(Operands);
}

bool M65832AsmParser::parseInstruction(ParseInstructionInfo &Info,
                                        StringRef Name, SMLoc NameLoc,
                                        OperandVector &Operands) {
  // Handle instruction suffixes like LD.L, LD.B, LD.W
  StringRef BaseMnemonic = Name;
  StringRef Suffix;
  
  size_t DotPos = Name.find('.');
  if (DotPos != StringRef::npos) {
    BaseMnemonic = Name.substr(0, DotPos);
    Suffix = Name.substr(DotPos);
  }
  
  // Add mnemonic as first operand (keep full name with suffix for matching)
  Operands.push_back(M65832Operand::createToken(Name, NameLoc));

  // If no more tokens, implied addressing
  if (getParser().getTok().is(AsmToken::EndOfStatement))
    return false;

  // Parse first operand
  if (!parseOperand(Operands, BaseMnemonic).isSuccess())
    return true;

  // Parse additional comma-separated operands
  while (getParser().getTok().is(AsmToken::Comma)) {
    getParser().Lex();
    if (!parseOperand(Operands, BaseMnemonic).isSuccess())
      return true;
  }

  if (!getParser().getTok().is(AsmToken::EndOfStatement))
    return Error(getParser().getTok().getLoc(), "unexpected token in operand");

  return false;
}

ParseStatus M65832AsmParser::parseDirective(AsmToken DirectiveID) {
  StringRef IDVal = DirectiveID.getIdentifier();

  // Handle M65832-specific directives
  if (IDVal == ".m8" || IDVal == ".m16" || IDVal == ".m32" ||
      IDVal == ".x8" || IDVal == ".x16" || IDVal == ".x32")
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
  (void)AsmOp;
  (void)Kind;
  return Match_InvalidOperand;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeM65832AsmParser() {
  RegisterMCAsmParser<M65832AsmParser> X(getTheM65832Target());
}

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "M65832GenAsmMatcher.inc"
