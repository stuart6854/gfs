cmake_minimum_required(VERSION 3.15)
project(gfs LANGUAGES CXX)

# Check if GFS is being used directly or via add_subdirectory
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(GFS_MASTER_PROJECT ON)
else()
    set(GFS_MASTER_PROJECT OFF)
endif()

option(GFS_BUILD_TESTS "Build test" ${GFS_MASTER_PROJECT})
option(GFS_BUILD_BENCHMARK "Build benchmark" ${GFS_MASTER_PROJECT})

option(LZ4_BUILD_CLI "Build lz4 program" OFF)
option(LZ4_BUILD_LEGACY_LZ4C "Build lz4c program with legacy argument support" OFF)
add_subdirectory(external/lz4/build/cmake)

add_library(gfs 
    src/gfs/filesystem.cpp
    src/gfs/binary_streams.cpp
)
target_include_directories(gfs PUBLIC include)
set_target_properties(gfs PROPERTIES 
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
target_link_libraries(gfs PRIVATE lz4_static)

if(${GFS_BUILD_TESTS})
    message(STATUS "Building testbed")
    add_subdirectory(testbed)
endif()

if(${GFS_BUILD_BENCHMARK})
    message(STATUS "Building benchmark")
    add_subdirectory(benchmark)
endif()