cmake_minimum_required(VERSION 3.5)
project(debug-plug-hsa-intercept C)

set(CMAKE_C_FLAGS                "${CMAKE_C_FLAGS} -Wall -std=c99 -pedantic -g")
set(CMAKE_C_STANDARD             99)

find_program(CLANG NAMES clang REQUIRED)
if(CLANG)
    message(STATUS "Found clang: ${CLANG}")
else()
    message(FATAL_ERROR "clang not found")
endif()
set(CMAKE_C_COMPILER            clang)

add_subdirectory(src)
