cmake_minimum_required(VERSION 3.22)

project(seat_allocator_vue_algo_benchmark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(benchmark CONFIG REQUIRED)

add_executable(seat_allocator_vue_algo_benchmark
    benchmark.cpp
)

target_include_directories(
    seat_allocator_vue_algo_benchmark PRIVATE
        ../src
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