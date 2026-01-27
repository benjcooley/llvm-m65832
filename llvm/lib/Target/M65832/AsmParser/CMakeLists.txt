add_llvm_component_library(LLVMM65832AsmParser
  M65832AsmParser.cpp

  LINK_COMPONENTS
  M65832Common
  M65832Desc
  M65832Info
  MC
  MCParser
  Support

  ADD_TO_COMPONENT
  M65832
  )

# Include path for common headers
target_include_directories(LLVMM65832AsmParser PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../Common
  )
