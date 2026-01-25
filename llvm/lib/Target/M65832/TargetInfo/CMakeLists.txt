add_llvm_component_library(LLVMM65832Info
  M65832TargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support

  ADD_TO_COMPONENT
  M65832
  )
