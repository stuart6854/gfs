add_executable(gfs_testbed
    testbed.cpp
)
target_include_directories(gfs_testbed PUBLIC include)
set_target_properties(gfs_testbed PROPERTIES 
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME testbed
)

target_link_libraries(gfs_testbed PRIVATE gfs)