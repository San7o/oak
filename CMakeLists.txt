cmake_minimum_required(VERSION 3.16)
project(oak
  VERSION 1.1.0
  DESCRIPTION "oak - a modern logger"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_program(CCACHE_PROGRAM ccache) # use ccache if available
if(CCACHE_PROGRAM)
  message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

if(MSVC)
  set(OAK_WARNING_FLAGS /W4 /permissive-)
  set(OAK_DEBUG_FLAGS /Zi /Od)
  set(OAK_RELEASE_FLAGS /O2)
  set(OAK_DEBUG_DEFINES _DEBUG)
elseif(MINGW)
  set(OAK_WARNING_FLAGS -Wall -Wextra -Wpedantic)
  set(OAK_DEBUG_FLAGS -O0 -ggdb)
  set(OAK_RELEASE_FLAGS -O3)
  set(OAK_DEBUG_DEFINES DEBUG)
else()
  set(OAK_WARNING_FLAGS -Wall -Wextra -Werror -Wpedantic)
  set(OAK_DEBUG_FLAGS -O0 -ggdb)
  set(OAK_RELEASE_FLAGS -O3)
  set(OAK_DEBUG_DEFINES DEBUG)
  set(OAK_DEBUG_LINK_FLAGS)
endif()

#
# Options
#

option(BUILD_SHARED_LIBS "Build shared library" ON)
option(OAK_BUILD_EXAMPLE "Build the example program" OFF)
option(OAK_BUILD_TESTS "Build the tests" OFF)
option(OAK_USE_SOCKETS "Enable logging on sockets" ON)

if (OAK_USE_SOCKETS)
  list(APPEND OAK_COMPILE_DEFINITIONS
    OAK_USE_SOCKETS)
endif()

#
# Sources and headers
#

set(OAK_SOURCES src/oak.cpp)
set(OAK_HEADERS include)

#
# Targets
#

# Library

if(BUILD_SHARED_LIBS)
  add_library(${PROJECT_NAME} SHARED
    ${OAK_SOURCES})
else()
  add_library(${PROJECT_NAME} STATIC
    ${OAK_SOURCES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
  ${OAK_HEADERS})
target_compile_options(${PROJECT_NAME} PRIVATE
  ${OAK_COMPILE_OPTIONS})
target_compile_definitions(${PROJECT_NAME} PRIVATE
  ${OAK_COMPILE_DEFINITIONS})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Configuring Debug build options...")

  target_compile_definitions(${PROJECT_NAME} PRIVATE
    ${OAK_DEBUG_DEFINES})
  target_compile_options(${PROJECT_NAME} PRIVATE
    ${OAK_DEBUG_FLAGS}
    ${OAK_WARNING_FLAGS})
  if(NOT MSVC)
    target_link_options(${PROJECT_NAME} PRIVATE
      ${OAK_DEBUG_LINK_FLAGS})
  endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Configuring Release build options...")
  target_compile_options(${PROJECT_NAME} PRIVATE
    ${OAK_RELEASE_FLAGS}
    ${OAK_WARNING_FLAGS})
endif()

# Example

if (OAK_BUILD_EXAMPLE)
  add_executable(${PROJECT_NAME}_example
    tests/example.cpp)
  target_include_directories(${PROJECT_NAME}_example PRIVATE
    ${OAK_HEADERS})
  target_compile_options(${PROJECT_NAME}_example PRIVATE
    ${OAK_DEBUG_FLAGS}
    ${OAK_WARNING_FLAGS})
  target_link_libraries(${PROJECT_NAME}_example PRIVATE
    ${PROJECT_NAME})
  target_compile_definitions(${PROJECT_NAME}_example PRIVATE
    ${OAK_DEBUG_DEFINES}
    ${OAK_COMPILE_DEFINITIONS})
endif()

# Tests

if(OAK_BUILD_TESTS)
  add_executable(${PROJECT_NAME}_tests
    tests/oak_tests.cpp)
  target_include_directories(${PROJECT_NAME}_tests PRIVATE
    tests
    ${OAK_HEADERS})
  target_compile_options(${PROJECT_NAME}_tests PRIVATE
    ${OAK_DEBUG_FLAGS}
    ${OAK_WARNING_FLAGS})
  target_link_libraries(${PROJECT_NAME}_tests PRIVATE
    ${PROJECT_NAME})
  target_compile_definitions(${PROJECT_NAME}_tests PRIVATE
    ${OAK_DEBUG_DEFINES}
    ${OAK_COMPILE_DEFINITIONS})
endif()
