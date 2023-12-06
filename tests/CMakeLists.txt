# ---- Dependencies ----
CPMAddPackage(
        NAME Catch2
        GITHUB_REPOSITORY catchorg/Catch2
        VERSION 3.4.0
)

# ---- Target ----

add_executable(gfs_tests tests.cpp)
target_link_libraries(gfs_tests PRIVATE gfs Catch2::Catch2WithMain)

# ---- Tests ----

include(CTest)

if (Catch2_ADDED)
    include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
    catch_discover_tests(gfs_tests)
endif ()