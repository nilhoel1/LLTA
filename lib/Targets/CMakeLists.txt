add_llvm_library(lltaTargets
  TargetRegistry.cpp
  MSP430/MSP430Target.cpp
  MSP430/MSP430FR5994Target.cpp
  MSP430/MSP430Pipeline.cpp
  MSP430/MSP430Options.cpp
  MSP430/FRAMWaitStatePass.cpp
  MSP430/FRAMCacheAnalysisPass.cpp
  DEPENDS LLVMAnalysis LLVMCodeGen LLVMCore LLVMSupport LLVMTarget
  LINK_LIBS TimingAnalysisBase lltaAnalysis lltaUtility
)
