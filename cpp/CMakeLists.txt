cmake_minimum_required(VERSION 4.3)
project(seat_allocator_vue_algo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(alloc_algo
    wasm-bridge.cpp
    src/algorithms/algorithm.cpp
    src/algorithms/constraints.cpp
    src/utils/dynamic_bitset.cpp
    src/utils/utils.cpp
)

target_include_directories(alloc_algo PRIVATE ${CMAKE_SOURCE_DIR}/src/)

target_link_libraries(alloc_algo PRIVATE
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

if(EMSCRIPTEN)
    message(STATUS "Building for WebAssembly with Emscripten")

    # Output alloc_algo.js and alloc_algo.wasm
    set_target_properties(alloc_algo PROPERTIES SUFFIX ".js")

    # Compile options (Crucial: Add --bind)
    target_compile_options(alloc_algo PRIVATE
        -O3
        -fexceptions
    )

    # Link options
    target_link_options(alloc_algo PRIVATE
        "-O3"
        "-sMODULARIZE=1"
        "-sEXPORT_ES6=1"
        "-sASSERTIONS=1" # Useful for debugging crashes
        "-lembind"        # Required for embind support
    )

    if(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "Release build: enabling additional optimizations")
        target_link_options(alloc_algo PRIVATE
            "SHELL:-s AGGRESSIVE_VARIABLE_ELIMINATION=1"
            "SHELL:-s ELIMINATE_DUPLICATE_FUNCTIONS=1"
        )
    endif()
else()
    message(WARNING "EMSCRIPTEN not found. Building native executable.")
endif()
