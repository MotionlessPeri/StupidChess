cmake_minimum_required(VERSION 3.24)

project(StupidChess VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(STUPIDCHESS_BUILD_SERVER "Build server target" ON)
option(STUPIDCHESS_BUILD_TESTS "Build tests" ON)

add_subdirectory(core)
add_subdirectory(protocol)

if(STUPIDCHESS_BUILD_SERVER)
  add_subdirectory(server)
endif()

if(STUPIDCHESS_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()
