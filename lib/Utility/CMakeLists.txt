message(STATUS "Found LLVM Machine Passes LIB for LLTA")

add_llvm_library(lltaUtility
  Options.cpp
  InstructionWords.cpp
  DEPENDS LLVMAnalysis LLVMCodeGen LLVMCore LLVMSupport LLVMTarget
)
