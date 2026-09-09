cmake_minimum_required(VERSION 3.22)
project(algo_optimizing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json REQUIRED)

set(SRC_HEADER_PATH ../../src)

add_executable(
    ${PROJECT_NAME}
        main.cpp
)

target_include_directories(
    ${PROJECT_NAME} PRIVATE
        ${SRC_HEADER_PATH}
)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE
        nlohmann_json::nlohmann_json
)

if(MINGW)
    target_link_libraries(
        ${PROJECT_NAME} PRIVATE
            -lstdc++exp
    )
endif()