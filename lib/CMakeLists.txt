add_subdirectory(MIRPasses)
add_subdirectory(Utility)
add_subdirectory(RTTargets)
add_subdirectory(ILP)
add_subdirectory(Analysis)

add_llvm_library(TimingAnalysisBase
  TimingAnalysisResults.cpp
  DEPENDS LLVMAnalysis LLVMCodeGen LLVMCore LLVMSupport LLVMTarget
)
