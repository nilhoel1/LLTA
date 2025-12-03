
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
    message(STATUS "Gurobi found - building with Gurobi optimizer support")
    add_definitions(-DENABLE_GUROBI)
    set(GUROBI_LIBS ${GUROBI_LIBRARIES})
  else()
    message(WARNING "Gurobi not found - building without Gurobi optimizer support")
    set(GUROBI_INCLUDE_DIRS "")
    set(GUROBI_LIBS "")
  endif()
else()
  message(STATUS "Gurobi support disabled")
  set(GUROBI_INCLUDE_DIRS "")
  set(GUROBI_LIBS "")
endif()

# Make HiGHS optional
option(LLTA_ENABLE_HIGHS "Enable HiGHS optimizer support" ON)

if(LLTA_ENABLE_HIGHS)
  find_package(HIGHS)
  if(HIGHS_FOUND)
    message(STATUS "HiGHS found - building with HiGHS optimizer support")
    add_definitions(-DENABLE_HIGHS)
    set(HIGHS_LIBS ${HIGHS_LIBRARIES})
  else()
    message(WARNING "HiGHS not found - building without HiGHS optimizer support")
    set(HIGHS_INCLUDE_DIRS "")
    set(HIGHS_LIBS "")
  endif()
else()
  message(STATUS "HiGHS support disabled")
  set(HIGHS_INCLUDE_DIRS "")
  set(HIGHS_LIBS "")
endif()

include_directories(include ${LLVM_BINARY_DIR}/lib/Target/ARM ${LLVM_SOURCE_DIR}/lib/Target/ARM ${LLVM_BINARY_DIR}/lib/Target/RISCV ${LLVM_SOURCE_DIR}/lib/Target/RISCV ${LLVM_BINARY_DIR}/lib/Target/MSP430 ${LLVM_SOURCE_DIR}/lib/Target/MSP430 ${GUROBI_INCLUDE_DIRS} ${HIGHS_INCLUDE_DIRS})

add_llvm_tool(llta
  LLTA.cpp
  NewPMDriver.cpp

  DEPENDS
  intrinsics_gen
  SUPPORT_PLUGINS
  )

add_subdirectory(lib)

target_link_libraries(llta PRIVATE lltaMPasses lltaUtility lltaRTTargets lltaILP TimingAnalysisBase ${GUROBI_LIBS} ${HIGHS_LIBS})

# Configure RPATH to find shared libraries at runtime
set(LLTA_RPATHS "")

if(HIGHS_FOUND)
  # Extract directory from the library path
  get_filename_component(HIGHS_LIB_DIR "${HIGHS_LIBRARIES}" DIRECTORY)
  list(APPEND LLTA_RPATHS "${HIGHS_LIB_DIR}")
endif()

if(GUROBI_FOUND)
  # GUROBI_LIBRARIES is a list, take the last element which is usually the C library
  list(GET GUROBI_LIBRARIES -1 GUROBI_MAIN_LIB)
  get_filename_component(GUROBI_LIB_DIR "${GUROBI_MAIN_LIB}" DIRECTORY)
  list(APPEND LLTA_RPATHS "${GUROBI_LIB_DIR}")
endif()

if(LLTA_RPATHS)
  set_target_properties(llta PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "${LLTA_RPATHS}"
    BUILD_RPATH "${LLTA_RPATHS}"
  )
endif()

export_executable_symbols_for_plugins(llta)
