cmake_minimum_required(VERSION 3.18)

project(
  dpdp
  VERSION 1.0
  DESCRIPTION ""
  HOMEPAGE_URL ""
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()
add_subdirectory(googletest)
add_subdirectory(gflags)
add_subdirectory(glog)
add_subdirectory(parlaylib)

add_compile_options(
  -O3
  -mcx16 -march=native
)

configure_file(config.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/PAM/include)

add_executable(post_office post_office.cpp)
target_link_libraries(post_office parlay gflags::gflags glog::glog)

add_executable(lcs lcs.cpp)
target_link_libraries(lcs parlay gflags::gflags glog::glog)
