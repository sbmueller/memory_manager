cmake_minimum_required(VERSION 3.9)
project(memory_manager)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")
add_library(memory_manager SHARED memoryManager.cpp)
find_package(GTest)
if(${GTEST_FOUND})
    message("GTest found, enabling testing")
    enable_testing()
    add_subdirectory(goodies/test)
endif(${GTEST_FOUND})
