message(STATUS "Found LLVM Analysis LIB for LLTA")

add_llvm_library(lltaAnalysis
  PipelineAnalysis.cpp
  Targets/MSP430Latency.cpp
  # SystemState.cpp # Reserved for future implementation
  # HardwareStrategies.cpp # Reserved for future implementation
  
  DEPENDS LLVMAnalysis LLVMCodeGen LLVMCore LLVMSupport LLVMTarget
  LINK_LIBS TimingAnalysisBase lltaRTTargets lltaUtility
)
