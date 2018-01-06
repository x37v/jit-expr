# Add subdirectories

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(parse)
include_directories(parse/)

# Add the binary and sources
add_executable(
  ../../parser
  parse/ast.h
  main.cc
  print.cc
)

target_link_libraries(../../parser parse)
