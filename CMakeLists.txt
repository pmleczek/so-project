cmake_minimum_required(VERSION 3.16)

project(so_project
  VERSION 1.0.0
  LANGUAGES CXX
)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMMON_SOURCES
    src/config.cpp
    src/output/Logger.cpp
    src/output/ResultsWriter.cpp
    src/ipc/MessageQueueManager.cpp
    src/ipc/SharedMemoryManager.cpp
    src/ipc/SemaphoreManager.cpp
    src/utils/random.cpp
    src/utils/constants.cpp
    src/utils/validation.cpp
)

add_executable(so_project_main 
    src/main/main.cpp
    src/main/MainProcess.cpp
    src/candidate/CandidateFactory.cpp
    ${COMMON_SOURCES}
)
target_include_directories(so_project_main PRIVATE include)
target_compile_features(so_project_main PRIVATE cxx_std_17)

add_executable(so_project_candidate
    src/candidate/main.cpp
    src/candidate/CandidateProcess.cpp
    ${COMMON_SOURCES}
)
target_include_directories(so_project_candidate PRIVATE include)
target_compile_features(so_project_candidate PRIVATE cxx_std_17)