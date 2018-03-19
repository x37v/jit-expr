#Minimum version of CMake to build this project
cmake_minimum_required(VERSION 3.0) #below 3 has a bug in mac os for c++11

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_MACOSX_RPATH Off)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9") #before 10.9 didn't see shared_ptr
set(CMAKE_OSX_ARCHITECTURES "i386;x86_64")

#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -W -Werror -pedantic -g -Wno-sign-compare -Wno-unused-parameter -Wno-overloaded-virtual -std=c++11")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -W -pedantic -Wno-unused-parameter -Wno-overloaded-virtual")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if (POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW)
endif ()

project(jit_expr)

#Defines subdirectory
add_subdirectory(src/)
