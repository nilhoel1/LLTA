message(STATUS "Found LLVM Machine Passes LIB for LLTA")

add_llvm_library(lltaUtility
  Options.cpp
  DEPENDS LLVMAnalysis LLVMCodeGen LLVMCore LLVMSupport LLVMTarget
)
