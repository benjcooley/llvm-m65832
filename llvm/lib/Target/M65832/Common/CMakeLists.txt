# M65832 Common Library - Shared ISA definitions and parser
#
# *** DO NOT EDIT THESE FILES DIRECTLY ***
#
# Source of truth: /Users/benjamincooley/projects/m65832/common/
# Sync script:     scripts/sync-m65832-common.sh
#
# See README.md in this directory for details.

add_llvm_component_library(LLVMM65832Common
  m65832_isa.c
  m65832_parser.c

  LINK_COMPONENTS
  Support

  ADD_TO_COMPONENT
  M65832
  )

# Allow C code in LLVM build
set_source_files_properties(
  m65832_isa.c
  m65832_parser.c
  PROPERTIES
  LANGUAGE C
  COMPILE_FLAGS "-std=c11"
  )
