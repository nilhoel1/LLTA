add_llvm_library(lltaGraph
  ProgramGraph.cpp
  DEPENDS LLVMAnalysis LLVMCodeGen LLVMCore LLVMSupport LLVMTarget
)
