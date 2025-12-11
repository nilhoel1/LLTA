message(STATUS "Found LLVM Machine Passes LIB for LLTA")

add_llvm_library(lltaRTTargets
  MSP430/MSP430MuArchState.cpp
  MSP430/MSP430Pipeline.cpp
  ProgramGraph.cpp
  DEPENDS LLVMAnalysis LLVMCodeGen LLVMCore LLVMSupport LLVMTarget
)
