add_llvm_library(lltaAnalysis
  AbstractStateGraph.cpp
  GraphAdapter.cpp
  WorklistSolver.cpp
  PipelineAnalysis.cpp
  InstructionCacheAnalysis.cpp
  Cache/CacheAnalysis.cpp
  Cache/FRAMAccessMapper.cpp


  DEPENDS
  intrinsics_gen

  LINK_LIBS
  lltaGraph
)
