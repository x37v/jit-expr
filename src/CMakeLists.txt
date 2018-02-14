project(jit_expr)

# Add subdirectories

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(pd.build/pd.cmake)

add_subdirectory(parse/)

#need the generated headers
set_pd_external_path(${CMAKE_BINARY_DIR})

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

file(GLOB licenses ../LICENSE*)
install(TARGETS jit_expr DESTINATION lib/pd-externals/jit_expr)
install(FILES jit_expr-help.pd DESTINATION lib/pd-externals/jit_expr)
install(FILES ${licenses} DESTINATION lib/pd-externals/jit_expr)
