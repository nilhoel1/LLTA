cmake_minimum_required(VERSION 3.10)
project(optimizer_optional_examples C)

# Find package modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# ==================== Gurobi Example ====================

option(ENABLE_GUROBI "Enable Gurobi support" ON)

if(ENABLE_GUROBI)
    find_package(GUROBI)

    if(GUROBI_FOUND)
        message(STATUS "Building Gurobi example with optimizer support")
        add_executable(gurobi_optional_example gurobi_optional_example.c)
        target_compile_definitions(gurobi_optional_example PRIVATE ENABLE_GUROBI)
        target_include_directories(gurobi_optional_example PRIVATE ${GUROBI_INCLUDE_DIRS})
        target_link_libraries(gurobi_optional_example ${GUROBI_C_LIBRARY})
    else()
        message(STATUS "Gurobi not found - building example without optimizer")
        add_executable(gurobi_optional_example gurobi_optional_example.c)
    endif()
else()
    message(STATUS "Gurobi support disabled - building example without optimizer")
    add_executable(gurobi_optional_example gurobi_optional_example.c)
endif()

# ==================== HiGHS Example ====================

option(ENABLE_HIGHS "Enable HiGHS support" ON)

if(ENABLE_HIGHS)
    find_package(HIGHS)

    if(HIGHS_FOUND)
        message(STATUS "Building HiGHS example with optimizer support")
        add_executable(highs_optional_example highs_optional_example.c)
        target_compile_definitions(highs_optional_example PRIVATE ENABLE_HIGHS)
        target_include_directories(highs_optional_example PRIVATE ${HIGHS_INCLUDE_DIRS})
        target_link_libraries(highs_optional_example ${HIGHS_LIBRARIES})
    else()
        message(STATUS "HiGHS not found - building example without optimizer")
        add_executable(highs_optional_example highs_optional_example.c)
    endif()
else()
    message(STATUS "HiGHS support disabled - building example without optimizer")
    add_executable(highs_optional_example highs_optional_example.c)
endif()

# Installation
install(TARGETS gurobi_optional_example highs_optional_example DESTINATION bin)
