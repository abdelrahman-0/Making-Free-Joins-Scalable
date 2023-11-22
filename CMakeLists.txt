cmake_minimum_required(VERSION 3.22)
project(making_free_joins_scalable)

set(CMAKE_CXX_STANDARD 17)
message("using ${CMAKE_CXX_COMPILER}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")

find_package(TBB REQUIRED)

add_executable(main main.cpp)
add_executable(benchmark_indexes benchmark_indexes.cpp)

include(includes/CMakeLists.txt)
include_directories(includes)
include(src/CMakeLists.txt)


target_link_libraries(main PUBLIC src_lib)
target_link_libraries(main PUBLIC duckdb_src)
target_link_libraries(main PUBLIC TBB::tbb)
target_link_libraries(main PUBLIC TBB::tbbmalloc)

target_link_libraries(benchmark_indexes PUBLIC src_lib)
target_link_libraries(benchmark_indexes PUBLIC duckdb_src)
target_link_libraries(benchmark_indexes PUBLIC TBB::tbb)
target_link_libraries(benchmark_indexes PUBLIC TBB::tbbmalloc)
