cmake_minimum_required(VERSION 3.20)
project(
    oak
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23 REQUIRED)
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

option(OAK_BUILD_TESTS "Build the tests" ON)
option(OAK_USE_SOCKETS "Enable logging on sockets" ON)
option(OAK_USE_CLANG "Use clang" OFF)

if(OAK_USE_CLANG)
    set(CMAKE_CXX_COMPILER clang++)
endif()

add_executable(oak src/main.cpp)
target_include_directories(oak PRIVATE include)
target_compile_options(oak PRIVATE -Wall -Wextra -Wpedantic
        -Werror -Wconversion -Wshadow)
if (OAK_USE_SOCKETS)
    target_compile_definitions(oak PRIVATE OAK_USE_SOCKETS)
endif()
if (OAK_USE_CLANG)
    target_compile_options(oak PRIVATE -std=c++23 -fexperimental-library) # jthread, format
    target_link_libraries(oak PRIVATE -fexperimental-library)
endif()

if(OAK_BUILD_TESTS)
    add_executable(tests tests/oak_tests.cpp)
    target_include_directories(tests PRIVATE include tests)
    target_compile_options(tests PRIVATE -Wall -Wextra -Wpedantic
            -Werror -Wconversion -Wshadow)
    if (OAK_USE_SOCKETS)
        target_compile_definitions(tests PRIVATE OAK_USE_SOCKETS)
    endif()
    if (OAK_USE_CLANG)
        target_compile_options(tests PRIVATE -std=c++23 -fexperimental-library)
        target_link_libraries(tests PRIVATE -fexperimental-library)
    endif()
endif()
