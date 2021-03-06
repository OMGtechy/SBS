cmake_minimum_required(VERSION 3.9)
project(SBS_BENCHMARK)

include(ExternalProject)

set(GBNCH_DIR ${CMAKE_BINARY_DIR}/benchmark/external/googlebenchmark)
set(GBNCH_BIN ${GBNCH_DIR}/bin)
set(GBNCH_SRC ${GBNCH_DIR}/source)
set(GBNCH_INC ${GBNCH_SRC}/include)
set(GBNCH_LIB ${GBNCH_BIN}/src)
unset(GIT_REPOSITORY)

ExternalProject_Add(
    googlebenchmark_benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG 674d0498b80097d8bc720eaa7980805b5332c7ad
    SOURCE_DIR "${GBNCH_SRC}"
    BINARY_DIR "${GBNCH_BIN}"
    INSTALL_COMMAND ""
    CMAKE_ARGS "-DBENCHMARK_ENABLE_GTEST_TESTS=OFF;-DBENCHMARK_ENABLE_TESTING=OFF;-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
)

link_directories(${GBNCH_LIB})

find_package(Threads REQUIRED)

add_executable(SBS_BENCHMARK source/benchmark_sbs_unscoped_stack_vector.cpp)
target_include_directories(SBS_BENCHMARK PRIVATE ../include ${GBNCH_INC})
add_dependencies(SBS_BENCHMARK googlebenchmark_benchmark)

target_compile_features(SBS_BENCHMARK PRIVATE cxx_std_17)
target_link_libraries(SBS_BENCHMARK benchmark Threads::Threads)

if(WIN32)
    list(APPEND CMAKE_CXX_FLAGS "/W4 /WX")
else(WIN32)
    list(APPEND CMAKE_CXX_FLAGS "-std=c++1z -Wall -Wextra -Wpedantic -Werror")
endif(WIN32)
