cmake_minimum_required(VERSION 3.16)
project(
    oak
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(OAK_SOURCES src/oak.cpp)
set(OAK_HEADERS include)
set(OAK_COMPILE_OPTIONS -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow)

option(BUILD_SHARED_LIBS "Build shared library" ON)
option(OAK_BUILD_EXAMPLES "Build the examples" ON)
option(OAK_BUILD_TESTS "Build the tests" ON)
option(OAK_USE_SOCKETS "Enable logging on sockets" ON)
option(OAK_USE_CLANG "Use clang" OFF)

if(OAK_USE_CLANG)
    set(CMAKE_CXX_COMPILER clang++)
endif()

if(BUILD_SHARED_LIBS)
  add_library(${PROJECT_NAME} SHARED ${OAK_SOURCES})
else()
  add_library(${PROJECT_NAME} STATIC ${OAK_SOURCES})
endif()
  
target_include_directories(${PROJECT_NAME} PRIVATE ${OAK_HEADERS})
target_compile_options(${PROJECT_NAME} PRIVATE ${OAK_COMPILE_OPTIONS})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME
  ${PROJECT_NAME})

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
