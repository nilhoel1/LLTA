cmake_minimum_required(VERSION 3.10)
project(gurobi_optional_example C)

# Check if Gurobi is available
option(ENABLE_GUROBI "Enable Gurobi support" ON)

if(ENABLE_GUROBI)
    # Try to find Gurobi
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")
    find_package(GUROBI)

    if(GUROBI_FOUND)
        message(STATUS "Building example with Gurobi support")
        add_definitions(-DENABLE_GUROBI)
        include_directories(${GUROBI_INCLUDE_DIRS})
        # Use GUROBI_C_LIBRARY for C code
        set(GUROBI_LIBS ${GUROBI_C_LIBRARY})
    else()
        message(STATUS "Gurobi not found - building example without optimizer")
        set(GUROBI_LIBS "")
    endif()
else()
    message(STATUS "Gurobi support disabled")
    set(GUROBI_LIBS "")
endif()

add_executable(gurobi_optional_example gurobi_optional_example.c)

if(GUROBI_FOUND)
    target_link_libraries(gurobi_optional_example ${GUROBI_LIBS})
endif()

# Installation
install(TARGETS gurobi_optional_example DESTINATION bin)
