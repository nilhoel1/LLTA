message(STATUS "Found LLVM Analysis LIB for LLTA")

add_llvm_library(lltaAnalysis
  PipelineAnalysis.cpp
  SystemState.cpp
  HardwareStrategies.cpp
  WorklistSolver.cpp
  Targets/MSP430Latency.cpp
  
  DEPENDS LLVMAnalysis LLVMCodeGen LLVMCore LLVMSupport LLVMTarget
  LINK_LIBS TimingAnalysisBase lltaRTTargets lltaUtility
)
