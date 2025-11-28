
#building out of llvm sources  is not possible, see:
#https://lists.llvm.org/pipermail/llvm-dev/2019-March/130706.html
message (STATUS "Found tool LLTA")

project("llta")

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(LLVM_LINK_COMPONENTS
  AllTargetsAsmParsers
  AllTargetsCodeGens
  AllTargetsDescs
  AllTargetsInfos
  Analysis
  AsmPrinter
  CodeGen
  CodeGenTypes
  Core
  IRPrinter
  IRReader
  MC
  MIRParser
  Passes
  Remarks
  ScalarOpts
  SelectionDAG
  Support
  Target
  TargetParser
  TransformUtils
  Vectorize
  )

message (STATUS "${PROJECT_BINARY_DIR}$")
message (STATUS "${PROJECT_SOURCE_DIR}$")

# Make Gurobi optional
option(LLTA_ENABLE_GUROBI "Enable Gurobi optimizer support" ON)

if(LLTA_ENABLE_GUROBI)
  find_package(GUROBI)
  if(GUROBI_FOUND)
    message(STATUS "Gurobi found - building with optimizer support")
    add_definitions(-DENABLE_GUROBI)
    set(GUROBI_LIBS ${GUROBI_CXX_LIBRARY})
  else()
    message(WARNING "Gurobi not found - building without optimizer support")
    set(GUROBI_INCLUDE_DIRS "")
    set(GUROBI_LIBS "")
  endif()
else()
  message(STATUS "Gurobi support disabled")
  set(GUROBI_INCLUDE_DIRS "")
  set(GUROBI_LIBS "")
endif()

include_directories(include ${LLVM_BINARY_DIR}/lib/Target/ARM ${LLVM_SOURCE_DIR}/lib/Target/ARM ${LLVM_BINARY_DIR}/lib/Target/RISCV ${LLVM_SOURCE_DIR}/lib/Target/RISCV ${LLVM_BINARY_DIR}/lib/Target/MSP430 ${LLVM_SOURCE_DIR}/lib/Target/MSP430 ${GUROBI_INCLUDE_DIRS})

add_llvm_tool(llta
  LLTA.cpp
  NewPMDriver.cpp

  DEPENDS
  intrinsics_gen
  SUPPORT_PLUGINS
  )

add_subdirectory(lib)

target_link_libraries(llta PRIVATE lltaMPasses lltaUtility lltaRTTargets TimingAnalysisBase ${GUROBI_LIBS})

export_executable_symbols_for_plugins(llta)
