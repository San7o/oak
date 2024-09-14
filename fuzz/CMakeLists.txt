cmake_minimum_required(VERSION 3.20)
project(
    oak_fuzz
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23 REQUIRED)

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

add_subdirectory(fuzztest)
set(CMAKE_CXX_COMPILER clang++)

option(OAK_USE_SOCKETS "Enable logging on sockets" ON)

add_executable(fuzz oak_fuzz.cpp)
target_include_directories(fuzz PRIVATE ../include)
target_link_libraries(fuzz PRIVATE -g -fsanitize-coverage=inline-8bit-counters
        -fsanitize-coverage=trace-cmp pthread -fsanitize=address)
target_compile_options(fuzz PRIVATE -fexperimental-library -fsanitize=address)
if (OAK_USE_SOCKETS)
    target_compile_definitions(fuzz PRIVATE OAK_USE_SOCKETS)
endif()
enable_testing()
link_fuzztest(fuzz)
gtest_discover_tests(fuzz)
