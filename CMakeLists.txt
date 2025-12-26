cmake_minimum_required(VERSION 3.16)

project(so_project
  VERSION 1.0.0
  LANGUAGES CXX
)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRC_FILES src/*.cpp)
add_executable(so_project ${SRC_FILES})
target_include_directories(so_project PRIVATE include)

target_compile_features(so_project PRIVATE cxx_std_17)
