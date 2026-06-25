message(STATUS "Found ILP Solver LIB for LLTA")

add_llvm_library(lltaILP
  AbstractHighsSolver.cpp
  PARTIAL_SOURCES_INTENDED
  DEPENDS LLVMCore LLVMSupport
  LINK_LIBS lltaGraph lltaAnalysis
)

# Link the HiGHS library if it is available
if(HIGHS_LIBS)
  target_link_libraries(lltaILP PRIVATE ${HIGHS_LIBS})
  target_include_directories(lltaILP PRIVATE ${HIGHS_INCLUDE_DIRS})
  target_compile_definitions(lltaILP PRIVATE ENABLE_HIGHS)
endif()
