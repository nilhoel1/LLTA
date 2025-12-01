message(STATUS "Found ILP Solver LIB for LLTA")

add_llvm_library(lltaILP
  ILPSolver.cpp
  GurobiSolver.cpp
  HighsSolver.cpp
  PARTIAL_SOURCES_INTENDED
  DEPENDS LLVMCore LLVMSupport
  LINK_LIBS lltaRTTargets
)
