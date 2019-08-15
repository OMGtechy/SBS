cmake_minimum_required(VERSION 3.9)
project(SBS_TEST)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(ExternalProject)

set_directory_properties(PROPERTIES EP_PREFIX ${CMAKE_BINARY_DIR}/external)
set(GTEST_DIR ${CMAKE_BINARY_DIR}/test/external/googletest)
set(GTEST_BIN ${GTEST_DIR}/bin)
set(GTEST_SRC ${GTEST_DIR}/source)
set(GTEST_INC ${GTEST_SRC}/googletest/include)
set(GTEST_LIB ${GTEST_BIN}/googlemock/gtest)

ExternalProject_Add(
    googletest_test
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG a325ad2db5deb623eab740527e559b81c0f39d65
    SOURCE_DIR "${GTEST_SRC}"
    BINARY_DIR "${GTEST_BIN}"
    INSTALL_COMMAND ""
)

link_directories(${GTEST_LIB})

find_package(Threads REQUIRED)

add_executable(SBS_TEST source/test_sbs_unscoped_stack_vector.cpp)
target_include_directories(SBS_TEST PRIVATE include ../include ${GTEST_INC})
add_dependencies(SBS_TEST googletest_test)

target_compile_features(SBS_TEST PRIVATE cxx_std_17)

if(WIN32)
    list(APPEND CMAKE_CXX_FLAGS "/W4 /WX")
    target_link_libraries(SBS_TEST debug Threads::Threads debug gtestd debug gtest_maind optimized gtest optimized gtest_main)
else(WIN32)
    list(APPEND CMAKE_CXX_FLAGS "-std=c++1z -Wall -Wextra -Wpedantic -Werror")
    target_link_libraries(SBS_TEST Threads::Threads gtest gtest_main)
endif(WIN32)
