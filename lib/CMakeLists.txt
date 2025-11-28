add_subdirectory(MIRPasses)
add_subdirectory(Utility)
add_subdirectory(RTTargets)

add_llvm_library(TimingAnalysisBase
  TimingAnalysisResults.cpp
  DEPENDS LLVMAnalysis LLVMCodeGen LLVMCore LLVMSupport LLVMTarget
)
