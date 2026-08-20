cmake_minimum_required(VERSION 3.22)

project(seat-allocator-algo-test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(GTest CONFIG REQUIRED)

enable_testing()

add_executable(unit-tests
    unit-test.cpp
)

target_include_directories(unit-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(unit-tests PRIVATE
    GTest::gtest_main
    GTest::gtest
)

include(GoogleTest)
gtest_discover_tests(unit-tests)