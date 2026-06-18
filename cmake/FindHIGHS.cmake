#### Find HiGHS library
# Looks for HiGHS in common installation locations

if (NOT HIGHS_FOUND)

    # Hardcoded search paths
    set(SEARCH_PATHS_FOR_HEADERS
            "$ENV{HIGHS_HOME}/include"
            "$ENV{HIGHS_HOME}/include/highs"
            "${CMAKE_CURRENT_SOURCE_DIR}/externalDeps/highs/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/externalDeps/highs/include/highs"
            "${CMAKE_CURRENT_SOURCE_DIR}/../externalDeps/highs/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/../externalDeps/highs/include/highs"
            "/usr/local/include/highs"
            "/usr/include/highs"
            "/opt/homebrew/include/highs"
            )

    set(SEARCH_PATHS_FOR_LIBRARIES
            "$ENV{HIGHS_HOME}/lib"
            "${CMAKE_CURRENT_SOURCE_DIR}/externalDeps/highs/lib"
            "${CMAKE_CURRENT_SOURCE_DIR}/../externalDeps/highs/lib"
            "/usr/local/lib"
            "/usr/lib"
            "/opt/homebrew/lib"
            )

    # Force static linking for HiGHS
    set(_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)

    find_path(HIGHS_INCLUDE_DIR Highs.h
            PATHS ${SEARCH_PATHS_FOR_HEADERS}
            NO_DEFAULT_PATH
            )

    # If not found with NO_DEFAULT_PATH, try system paths
    if(NOT HIGHS_INCLUDE_DIR)
        find_path(HIGHS_INCLUDE_DIR Highs.h
                PATHS ${SEARCH_PATHS_FOR_HEADERS}
                )
    endif()

    find_library(HIGHS_LIBRARY
            NAMES highs libhighs
            PATHS ${SEARCH_PATHS_FOR_LIBRARIES}
            NO_DEFAULT_PATH
            )

    # If not found with NO_DEFAULT_PATH, try system paths
    if(NOT HIGHS_LIBRARY)
        find_library(HIGHS_LIBRARY
                NAMES highs libhighs
                PATHS ${SEARCH_PATHS_FOR_LIBRARIES}
                )
    endif()

    # Restore original library suffixes
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})

    # setup header file directories
    set(HIGHS_INCLUDE_DIRS ${HIGHS_INCLUDE_DIR})

    # setup library files
    set(HIGHS_LIBRARIES ${HIGHS_LIBRARY})

endif ()

# Check that HiGHS was successfully found
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HIGHS DEFAULT_MSG HIGHS_INCLUDE_DIRS HIGHS_LIBRARIES)

# Hide variables from CMake-Gui options
mark_as_advanced(
        HIGHS_INCLUDE_DIRS
        HIGHS_INCLUDE_DIR
        HIGHS_LIBRARIES
        HIGHS_LIBRARY
)
