add_llvm_library(lltaAnalysis
  AbstractStateGraph.cpp
  GraphAdapter.cpp
  WorklistSolver.cpp
  PipelineAnalysis.cpp


  DEPENDS
  intrinsics_gen

  LINK_LIBS
  lltaRTTargets
)
