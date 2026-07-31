cmake_minimum_required(VERSION 4.3)

project(seat_allocator_vue_algo_benchmark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

find_package(benchmark CONFIG REQUIRED)

add_executable(seat_allocator_vue_algo_benchmark
    test.cpp
)

target_sources(
    seat_allocator_vue_algo_benchmark PRIVATE
    FILE_SET CXX_MODULES
    BASE_DIRS ../src
    FILES
        ../src/constraints.cppm
        ../src/shuffler.cppm
        ../src/utils.cppm
        ../src/dynamic-bitset.cppm
        ../src/grid.cppm
)

target_compile_definitions(
    seat_allocator_vue_algo_benchmark PRIVATE
        BENCHMARKING
)

target_link_libraries(
    seat_allocator_vue_algo_benchmark PRIVATE
        benchmark::benchmark
        -lstdc++exp
)