#Minimum version of CMake to build this project
cmake_minimum_required(VERSION 3.0) #below 3 has a bug in mac os for c++11


if (POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW)
endif ()

project(jit_expr)

#Defines subdirectory
add_subdirectory(src/)
