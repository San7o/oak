cmake_minimum_required(VERSION 3.20)
project(
    oak
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23 REQUIRED)

option(OAK_BUILD_TESTS "Build the tests" ON)
option(OAK_USE_SOCKETS "Enable logging on sockets" ON)
option(OAK_USE_CLANG "Use clang" OFF) # not supported yet

# TODO: The library currently does not support clang
if(OAK_USE_CLANG)
    set(CMAKE_CXX_COMPILER clang++)
endif()

add_executable(oak src/main.cpp)
target_include_directories(oak PRIVATE include)
target_compile_options(oak PRIVATE -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow)
if (OAK_USE_SOCKETS)
    target_compile_definitions(oak PRIVATE OAK_USE_SOCKETS)
endif()
if (OAK_USE_CLANG)
    target_compile_options(oak PRIVATE -fexperimental-library -std=c++23) # jthread, format
endif()

if(OAK_BUILD_TESTS)
    add_executable(tests tests/oak_tests.cpp)
    target_include_directories(tests PRIVATE include tests)
    target_compile_options(tests PRIVATE -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow)
    if (OAK_USE_SOCKETS)
        target_compile_definitions(tests PRIVATE OAK_USE_SOCKETS)
    endif()
    if (OAK_USE_CLANG)
        target_compile_options(tests PRIVATE -fexperimental-library)
    endif()
endif()
