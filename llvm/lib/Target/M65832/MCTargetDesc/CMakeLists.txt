add_llvm_component_library(LLVMM65832Desc
  M65832AsmBackend.cpp
  M65832ELFObjectWriter.cpp
  M65832InstPrinter.cpp
  M65832MCAsmInfo.cpp
  M65832MCCodeEmitter.cpp
  M65832MCTargetDesc.cpp

  LINK_COMPONENTS
  MC
  Support
  M65832Info

  ADD_TO_COMPONENT
  M65832
  )
