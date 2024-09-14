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

set(OAK_SOURCES src/oak.cpp)
set(OAK_HEADERS include)
set(OAK_COMPILE_OPTIONS -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow)

option(OAK_BUILD_SHARED "Build shared library" ON)
option(OAK_BUILD_STATIC "Build static library" OFF)
option(OAK_BUILD_EXAMPLES "Build the examples" ON)
option(OAK_BUILD_TESTS "Build the tests" ON)
option(OAK_USE_SOCKETS "Enable logging on sockets" ON)
option(OAK_USE_CLANG "Use clang" OFF)

if(OAK_USE_CLANG)
    set(CMAKE_CXX_COMPILER clang++)
endif()

if(OAK_BUILD_SHARED)
    add_library(oak SHARED ${OAK_SOURCES})
    target_include_directories(oak PRIVATE ${OAK_HEADERS})
    target_compile_options(oak PRIVATE ${OAK_COMPILE_OPTIONS})
endif()

if(OAK_BUILD_STATIC)
    add_library(oak_static STATIC ${OAK_SOURCES})
    target_include_directories(oak_static PRIVATE ${OAK_HEADERS})
    target_compile_options(oak_static PRIVATE ${OAK_COMPILE_OPTIONS})
endif()

if (OAK_BUILD_EXAMPLES)

    add_executable(example src/main.cpp ${OAK_SOURCES})
    target_include_directories(example PRIVATE ${OAK_HEADERS})
    target_compile_options(example PRIVATE ${OAK_COMPILE_OPTIONS})

    if (OAK_USE_SOCKETS)
        target_compile_definitions(example PRIVATE OAK_USE_SOCKETS)
    endif()
    if (OAK_USE_CLANG)
        target_compile_options(example PRIVATE -std=c++23 -fexperimental-library) # jthread, format
        target_link_libraries(example PRIVATE -fexperimental-library)
    endif()
endif()

if(OAK_BUILD_TESTS)
    add_executable(tests tests/oak_tests.cpp ${OAK_SOURCES})
    target_include_directories(tests PRIVATE tests ${OAK_HEADERS})
    target_compile_options(tests PRIVATE ${OAK_COMPILE_OPTIONS})
    if (OAK_USE_SOCKETS)
        target_compile_definitions(tests PRIVATE OAK_USE_SOCKETS)
    endif()
    if (OAK_USE_CLANG)
        target_compile_options(tests PRIVATE -std=c++23 -fexperimental-library)
        target_link_libraries(tests PRIVATE -fexperimental-library)
    endif()
endif()
