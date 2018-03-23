project(jit_expr)

# The version number.
set(jit_expr_VERSION_MAJOR 0)
set(jit_expr_VERSION_MINOR 1)
set(jit_expr_VERSION_PATCH 1)

configure_file(
  "${PROJECT_SOURCE_DIR}/jit_expr_version.h.in"
  "${PROJECT_BINARY_DIR}/jit_expr_version.h"
)

# Add subdirectories

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(pd.build/pd.cmake)

add_subdirectory(parse/)

# http://llvm.org/docs/CMake.html#embedding-llvm-in-your-project
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(parse)
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
#need the generated headers
include_directories(${CMAKE_CURRENT_BINARY_DIR}/parse)
include_directories("${PROJECT_BINARY_DIR}")

add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs support core irreader mcjit linker native)

#setup printer
add_executable(
  printer
  parse/ast.h
  main.cc
  print.cc
)
target_link_libraries(printer parse ${llvm_libs} m)

#setup external
set(OUT_DIR ${CMAKE_BINARY_DIR}/jit_expr)
set_pd_external_path(${OUT_DIR})
file(GLOB LICENSES ../LICENSE*)
file(COPY ${LICENSES} DESTINATION ${OUT_DIR})
#process the help file to add the version number
configure_file(
  "jit_expr-help.pd"
  "${OUT_DIR}/jit_expr-help.pd"
)
file(GLOB jit_expr_sources llvmcodegen/codegen.cc jit_expr.cpp)
add_pd_external(jit_expr jit_expr "${jit_expr_sources}")
target_link_libraries(jit_expr parse ${llvm_libs} m)
