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
        "--emit-tsd" "${CMAKE_CURRENT_BINARY_DIR}/alloc_algo.d.ts"
    )

    if(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "Release build: enabling additional optimizations")
        target_link_options(alloc_algo PRIVATE
            "SHELL:-s AGGRESSIVE_VARIABLE_ELIMINATION=1"
            "SHELL:-s ELIMINATE_DUPLICATE_FUNCTIONS=1"
        )
    endif()

    set(VUE_PROJECT_DIR "${CMAKE_SOURCE_DIR}/..")

    set(WASM_COPY_SOURCES
        "$<TARGET_FILE:alloc_algo>"
        "$<TARGET_FILE_DIR:alloc_algo>/alloc_algo.wasm"
        "${CMAKE_CURRENT_BINARY_DIR}/alloc_algo.d.ts"
    )
    set(WASM_COPY_DESTS
        "${VUE_PROJECT_DIR}/src/assets/wasm/alloc_algo.js"
        "${VUE_PROJECT_DIR}/public/alloc_algo.wasm"
        "${VUE_PROJECT_DIR}/src/assets/wasm/alloc_algo.d.ts"
    )

    set(WASM_COPY_COMMANDS)
    foreach(src dst IN ZIP_LISTS WASM_COPY_SOURCES WASM_COPY_DESTS)
        list(APPEND WASM_COPY_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${src}" "${dst}")
    endforeach()

    add_custom_command(TARGET alloc_algo POST_BUILD
        ${WASM_COPY_COMMANDS}
        COMMENT "Copying WASM outputs to Vue project directories"
    )
else()
    message(
        FATAL_ERROR
            "EMSCRIPTEN not found. This project is a WebAssembly-only module: "
            "wasm-bridge.cpp is an embind bridge and has no main(). "
            "Please configure with the Emscripten toolchain, e.g.: "
            "cmake --preset wasm (make sure EMSDK is set)."
    )
endif()
