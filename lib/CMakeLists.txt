add_subdirectory(Graph)
add_subdirectory(Targets)
add_subdirectory(MIRPasses)
add_subdirectory(Utility)
add_subdirectory(ILP)
add_subdirectory(Analysis)
add_subdirectory(Pipeline)

add_llvm_library(TimingAnalysisBase
  TimingAnalysisResults.cpp
  DEPENDS LLVMAnalysis LLVMCodeGen LLVMCore LLVMSupport LLVMTarget
)
