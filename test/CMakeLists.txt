cmake_minimum_required(VERSION 3.9)
project(SBS_TEST)

include(ExternalProject)

set_directory_properties(PROPERTIES EP_PREFIX ${CMAKE_BINARY_DIR}/external)
set(CMAKE_CXX_STANDARD_REQUIRED 17)
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

list(APPEND CMAKE_CXX_FLAGS "-std=c++1z -Wall -Wextra -Wpedantic -Werror")

add_executable(SBS_TEST source/test_sbs_unscoped_stack_vector.cpp)
target_include_directories(SBS_TEST PRIVATE include ../include ${GTEST_INC})
add_dependencies(SBS_TEST googletest_test)

target_link_libraries(SBS_TEST Threads::Threads gtest gtest_main)
