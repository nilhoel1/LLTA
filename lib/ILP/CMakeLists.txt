message(STATUS "Found ILP Solver LIB for LLTA")

add_llvm_library(lltaILP
  ILPSolver.cpp
  GurobiSolver.cpp
  HighsSolver.cpp
  PARTIAL_SOURCES_INTENDED
  DEPENDS LLVMCore LLVMSupport
  LINK_LIBS lltaRTTargets
)

# Link Gurobi and HiGHS libraries if they are available
if(GUROBI_LIBS)
  target_link_libraries(lltaILP PRIVATE ${GUROBI_LIBS})
  target_include_directories(lltaILP PRIVATE ${GUROBI_INCLUDE_DIRS})
endif()

if(HIGHS_LIBS)
  target_link_libraries(lltaILP PRIVATE ${HIGHS_LIBS})
  target_include_directories(lltaILP PRIVATE ${HIGHS_INCLUDE_DIRS})
endif()
