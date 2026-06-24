# Standalone unit tests for the modular cache analysis (no GoogleTest).
add_llvm_executable(LLTACacheModuleTests
  CacheModuleTests.cpp
  PARTIAL_SOURCES_INTENDED
)
# Header-only modules pull in llvm/ADT/StringRef.h; link Support for its symbols.
target_link_libraries(LLTACacheModuleTests PRIVATE LLVMSupport)

# Register with CTest...
add_test(NAME LLTACacheModuleTests COMMAND LLTACacheModuleTests)

# ...and provide a convenience target that builds and runs them directly:
#   cmake --build <build> --target check-llta-cache
add_custom_target(check-llta-cache
  COMMAND LLTACacheModuleTests
  DEPENDS LLTACacheModuleTests
  COMMENT "Running LLTA cache module unit tests"
)

# --- CFG / ProgramGraph construction tests -------------------------------
# Exercise the legacy ProgramGraph data structure (lib/Graph) in isolation.
add_llvm_executable(LLTAProgramGraphTests
  ProgramGraphTests.cpp
  PARTIAL_SOURCES_INTENDED
)
target_link_libraries(LLTAProgramGraphTests PRIVATE lltaGraph)
add_test(NAME LLTAProgramGraphTests COMMAND LLTAProgramGraphTests)
add_custom_target(check-llta-cfg
  COMMAND LLTAProgramGraphTests
  DEPENDS LLTAProgramGraphTests
  COMMENT "Running LLTA CFG/ProgramGraph unit tests"
)

# End-to-end fillGraphWithFunction() on a synthetic MachineFunction (the empty /
# no-return border cases unreachable through the C->ELF toolchain). Needs the
# LLVM CodeGen stack to fabricate a MachineFunction with a Bogus target.
set(LLVM_LINK_COMPONENTS
  CodeGen
  CodeGenTypes
  Core
  MC
  SelectionDAG
  Support
  Target
  TargetParser
  TransformUtils
)
add_llvm_executable(LLTAMachineFunctionGraphTests
  MachineFunctionGraphTests.cpp
  PARTIAL_SOURCES_INTENDED
)
target_link_libraries(LLTAMachineFunctionGraphTests PRIVATE lltaGraph)
add_test(NAME LLTAMachineFunctionGraphTests
  COMMAND LLTAMachineFunctionGraphTests)
add_dependencies(check-llta-cfg LLTAMachineFunctionGraphTests)

# --- WCET ILP formulation tests ------------------------------------------
# Build AbstractStateGraphs by hand and assert the HiGHS solver's WCET/Status.
add_llvm_executable(LLTAILPSolverTests
  ILPSolverTests.cpp
  PARTIAL_SOURCES_INTENDED
)
target_link_libraries(LLTAILPSolverTests PRIVATE lltaILP lltaAnalysis)
# The solver only runs when an ILP backend is enabled; mirror lltaILP's
# ENABLE_HIGHS define so the test knows whether to assert or skip.
if(HIGHS_LIBS)
  target_compile_definitions(LLTAILPSolverTests PRIVATE ENABLE_HIGHS)
endif()
add_test(NAME LLTAILPSolverTests COMMAND LLTAILPSolverTests)
add_custom_target(check-llta-ilp
  COMMAND LLTAILPSolverTests
  DEPENDS LLTAILPSolverTests
  COMMENT "Running LLTA WCET ILP unit tests"
)
