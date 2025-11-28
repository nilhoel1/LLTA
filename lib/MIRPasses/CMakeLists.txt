message(STATUS "Found LLVM Machine Passes LIB for LLTA")

add_llvm_library(lltaMPasses
  AsmDumpAndCheckPass.cpp
  TimingAnalysisPasses.cpp
  AdressResolverPass.cpp
  InstructionLatencyPass.cpp
  AccessAnalyses.cpp
  PathAnalysisPass.cpp
  MachineLoopBoundAgregatorPass.cpp
  FillMuGraphPass.cpp
  Mir2IrPass.cpp
  CallSplitterPass.cpp
  DebugIRPass.cpp
  DEPENDS LLVMAnalysis LLVMCodeGen LLVMCore LLVMSupport LLVMTarget
  LINK_LIBS TimingAnalysisBase lltaRTTargets lltaUtility
)
