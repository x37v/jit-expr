# Add subdirectories

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(parse)

# Add the binary and sources
add_executable(
  ../../parser
  main.cc
)

target_link_libraries(../../parser parse)
