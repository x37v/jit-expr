# Add subdirectories

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(parse)
include_directories(parse/)

# http://llvm.org/docs/CMake.html#embedding-llvm-in-your-project
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Add the binary and sources
add_executable(
  ../../parser
  parse/ast.h
  llvmcodegen/codegen.cc
  llvmcodegen/codegen.h
  main.cc
  print.cc
)

llvm_map_components_to_libnames(llvm_libs support core irreader mcjit linker)

target_link_libraries(../../parser parse ${llvm_libs})
