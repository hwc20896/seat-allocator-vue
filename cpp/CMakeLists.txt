cmake_minimum_required(VERSION 3.22)
project(seat_allocator_vue_algo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(alloc_algo
    wasm-bridge.cpp
)

target_include_directories(
    alloc_algo PRIVATE
    ${CMAKE_SOURCE_DIR}/src/
)

if(EMSCRIPTEN)
    message(STATUS "Building for WebAssembly with Emscripten")

    set_target_properties(alloc_algo PROPERTIES SUFFIX ".js")

    target_link_options(alloc_algo PRIVATE
        "-O3"
        "-sMODULARIZE=1"
        "-sEXPORT_ES6=1"
        "-sASSERTIONS=1" # Useful for debugging crashes
        "-lembind"        # Required for embind support
        "-sASYNCIFY_STACK_SIZE=262144"  # 256kB
        "-sASYNCIFY"
    )

    if(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "Release build: enabling additional optimizations")
        target_link_options(alloc_algo PRIVATE
            "SHELL:-s AGGRESSIVE_VARIABLE_ELIMINATION=1"
            "SHELL:-s ELIMINATE_DUPLICATE_FUNCTIONS=1"
        )
    endif()

    set(VUE_PROJECT_DIR "${CMAKE_SOURCE_DIR}/..")

    add_custom_command(TARGET alloc_algo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "$<TARGET_FILE:alloc_algo>"
        "${VUE_PROJECT_DIR}/src/assets/wasm/alloc_algo.js"
        COMMAND ${CMAKE_COMMAND} -E copy
        "$<TARGET_FILE_DIR:alloc_algo>/alloc_algo.wasm"
        "${VUE_PROJECT_DIR}/public/alloc_algo.wasm"
        COMMENT "Copying WASM outputs to Vue project directories"
    )
else()
    message(WARNING "EMSCRIPTEN not found. Building native executable.")
endif()
