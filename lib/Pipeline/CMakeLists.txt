add_llvm_library(lltaPipeline
  HardwarePipeline.cpp

  DEPENDS
  intrinsics_gen
)
