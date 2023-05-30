cmake_minimum_required(VERSION 3.20)

project(
  dc3
  VERSION 1.0
  DESCRIPTION ""
  HOMEPAGE_URL ""
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(gflags)
add_subdirectory(glog)
add_subdirectory(parlaylib)

configure_file(dc3_config.h.in dc3_config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(dc3_test dc3_test.cpp)
target_link_libraries(dc3_test PRIVATE parlay gflags::gflags glog::glog)