project(jit_expr)

# Add subdirectories

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_MACOSX_RPATH Off)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.4)
set(CMAKE_OSX_ARCHITECTURES "i386;x86_64")

include(pd.build/pd.cmake)

add_subdirectory(parse/)

#set_pd_sources(${project_source_dir}/pure-data/src)
set_pd_sources(/home/alex/local/src/pure-data/src/)
set_pd_external_path(externals/)

# http://llvm.org/docs/CMake.html#embedding-llvm-in-your-project
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(parse)
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/parse)

add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs support core irreader mcjit linker native)

file(GLOB jit_expr_sources llvmcodegen/codegen.cc jit_expr.cpp)
add_pd_external(jit_expr jit_expr "${jit_expr_sources}")

target_link_libraries(jit_expr parse ${llvm_libs} m)
