add_llvm_component_group(M65832)

set(LLVM_TARGET_DEFINITIONS M65832.td)

tablegen(LLVM M65832GenRegisterInfo.inc -gen-register-info)
tablegen(LLVM M65832GenInstrInfo.inc -gen-instr-info)
tablegen(LLVM M65832GenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM M65832GenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM M65832GenDAGISel.inc -gen-dag-isel)
tablegen(LLVM M65832GenCallingConv.inc -gen-callingconv)
tablegen(LLVM M65832GenSubtargetInfo.inc -gen-subtarget)
# tablegen(LLVM M65832GenMCCodeEmitter.inc -gen-emitter)  # TODO: Add when encoding is defined
tablegen(LLVM M65832GenMCPseudoLowering.inc -gen-pseudo-lowering)

add_public_tablegen_target(M65832CommonTableGen)

add_llvm_target(M65832CodeGen
  M65832AsmPrinter.cpp
  M65832FrameLowering.cpp
  M65832InstrInfo.cpp
  M65832ISelDAGToDAG.cpp
  M65832ISelLowering.cpp
  M65832MCInstLower.cpp
  M65832RegisterInfo.cpp
  M65832Subtarget.cpp
  M65832TargetMachine.cpp
  M65832TargetObjectFile.cpp

  LINK_COMPONENTS
  Analysis
  AsmPrinter
  CodeGen
  CodeGenTypes
  Core
  MC
  M65832Desc
  M65832Info
  SelectionDAG
  Support
  Target
  TransformUtils

  ADD_TO_COMPONENT
  M65832
  )

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
