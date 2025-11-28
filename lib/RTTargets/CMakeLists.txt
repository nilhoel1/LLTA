message(STATUS "Found LLVM Machine Passes LIB for LLTA")

add_llvm_library(lltaRTTargets
  MSP430/MSP430MuArchState.cpp
  MuArchStateGraph.cpp
  DEPENDS LLVMAnalysis LLVMCodeGen LLVMCore LLVMSupport LLVMTarget
)
