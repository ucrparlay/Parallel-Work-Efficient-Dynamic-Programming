cmake_minimum_required(VERSION 3.15)

project(
  ttt
  VERSION 1.0
  DESCRIPTION "ttt"
  HOMEPAGE_URL "ttt"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(parlaylib)

add_executable(dc3_test dc3_test.cpp)
target_link_libraries(dc3_test PUBLIC parlay)