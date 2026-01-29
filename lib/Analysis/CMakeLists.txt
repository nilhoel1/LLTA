add_llvm_library(lltaAnalysis
  AbstractStateGraph.cpp
  Callbacks/CallbackManager.cpp
  Callbacks/DebugLoggingCallback.cpp
  Callbacks/EdgeJoinabilityCallback.cpp
  Callbacks/CacheMissCallback.cpp
  Callbacks/ConstraintGenerationCallback.cpp
  Callbacks/VisualizationCallback.cpp
  GraphAdapter.cpp
  WorklistSolver.cpp
  PipelineAnalysis.cpp
  Targets/MSP430Latency.cpp

  DEPENDS
  intrinsics_gen

  LINK_LIBS
  lltaRTTargets
)

target_include_directories(lltaAnalysis PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/Callbacks
)
