add_executable(gfs_benchmark
    benchmark.cpp
)
target_include_directories(gfs_benchmark PUBLIC include)
set_target_properties(gfs_benchmark PROPERTIES 
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME benchmark
)

target_link_libraries(gfs_benchmark PRIVATE gfs)