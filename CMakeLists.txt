cmake_minimum_required(VERSION 3.20)

project(
  dc3
  VERSION 1.0
  DESCRIPTION ""
  HOMEPAGE_URL ""
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(gflags)
add_subdirectory(parlaylib)

configure_file(config.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(post_office post_office.cpp)
target_link_libraries(post_office parlay gflags::gflags)
