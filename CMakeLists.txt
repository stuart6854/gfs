cmake_minimum_required(VERSION 3.15)
project(gfs LANGUAGES CXX)

# ---- Include guards ----

if (PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(
        FATAL_ERROR
        "GFS - In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
    )
endif ()

# ---- ----

# Check if GFS is being used directly or via add_subdirectory
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(GFS_MASTER_PROJECT ON)
else ()
    set(GFS_MASTER_PROJECT OFF)
endif ()

# ---- Configuration ----

option(GFS_BUILD_TESTS "Build test" ${GFS_MASTER_PROJECT})

# ---- Dependencies ----

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/CPM.cmake)

CPMAddPackage(
    NAME filewatch
    GIT_TAG a59891baf375b73ff28144973a6fafd3fe40aa21
    GITHUB_REPOSITORY ThomasMonkman/filewatch
    OPTIONS
    "BuildTests OFF"
)
if (filewatch_ADDED)
    add_library(filewatch INTERFACE)
    target_include_directories(filewatch SYSTEM INTERFACE ${filewatch_SOURCE_DIR})
endif ()

CPMAddPackage(
    NAME nlohmann_json
    VERSION 3.11.3
    # the git repo is incredibly large, so we download the archived include directory
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/include.zip
    URL_HASH SHA256=a22461d13119ac5c78f205d3df1db13403e58ce1bb1794edc9313677313f4a9d
)
if (nlohmann_json_ADDED)
    add_library(nlohmann_json INTERFACE IMPORTED)
    target_include_directories(nlohmann_json INTERFACE ${nlohmann_json_SOURCE_DIR}/include)
endif ()

CPMAddPackage("gh:fraillt/bitsery#v5.2.3")

CPMAddPackage(
    NAME lz4
    GIT_REPOSITORY https://github.com/lz4/lz4.git
    VERSION 1.9.4
    DOWNLOAD_ONLY YES
)
if (lz4_ADDED)
    option(LZ4_BUILD_CLI "Build lz4 program" OFF)
    option(LZ4_BUILD_LEGACY_LZ4C "Build lz4c program with legacy argument support" OFF)
    add_subdirectory(${lz4_SOURCE_DIR}/build/cmake ${lz4_BINARY_DIR} EXCLUDE_FROM_ALL SYSTEM)
endif ()

# ---- Define GFS library ----

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files
# automatically. Keep that in mind when changing files, or explicitly mention them here.
file(GLOB_RECURSE GFS_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB_RECURSE GFS_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(gfs ${GFS_HEADERS} ${GFS_SOURCE})
set(GFS_COMPILE_FLAGS

)
target_compile_options(${PROJECT_NAME}
    PUBLIC
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror -Wall -Wextra $<$<CONFIG:Debug>:-Wpedantic -pedantic-errors>>
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
)


target_include_directories(gfs PRIVATE include)
target_include_directories(gfs SYSTEM INTERFACE include)

# Link dependencies
target_link_libraries(gfs
    PRIVATE nlohmann_json Bitsery::bitsery lz4_static
    PUBLIC filewatch
)

# ---- Tests ----

if (${GFS_BUILD_TESTS})
    message(STATUS "GFS - Enabling tests")
    add_subdirectory(tests)
endif ()