cmake_minimum_required(VERSION 3.16)

project(so_project
  VERSION 1.0.0
  LANGUAGES CXX
)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMMON_SOURCES
    src/common/ipc/SemaphoreManager.cpp
    src/common/ipc/SharedMemoryManager.cpp
    src/common/output/Logger.cpp
    src/common/utils/Random.cpp
)

add_executable(dean 
    src/dean/main.cpp
    ${COMMON_SOURCES}
)
target_include_directories(dean PRIVATE include)
target_compile_features(dean PRIVATE cxx_std_17)

add_executable(candidate
    src/candidate/main.cpp
    ${COMMON_SOURCES}
)
target_include_directories(candidate PRIVATE include)
target_compile_features(candidate PRIVATE cxx_std_17)

add_executable(commission
    src/commission/main.cpp
    src/commission/CommissionProcess.cpp
    ${COMMON_SOURCES}
)
target_include_directories(commission PRIVATE include)
target_compile_features(commission PRIVATE cxx_std_17)
