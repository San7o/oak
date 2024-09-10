cmake_minimum_required(VERSION 3.20)
project(
    oak
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23 REQUIRED)

option(OAK_BUILD_TESTS "Build the tests" ON)

add_executable(oak src/main.cpp)
target_include_directories(oak PRIVATE include)

if(OAK_BUILD_TESTS)
    add_executable(tests tests/oak_tests.cpp)
    target_include_directories(tests PRIVATE include tests)
endif()
