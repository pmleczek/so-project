cmake_minimum_required(VERSION 3.16)

project(so_project
  VERSION 1.0.0
  LANGUAGES CXX
)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(COMMON_SOURCES

# )

add_executable(dean 
    src/dean/main.cpp
    # src/main/MainProcess.cpp
    # src/candidate/CandidateFactory.cpp
    # ${COMMON_SOURCES}
)
target_include_directories(dean PRIVATE include)
target_compile_features(dean PRIVATE cxx_std_17)

# add_executable(so_project_candidate
#     src/candidate/main.cpp
#     src/candidate/CandidateProcess.cpp
#     ${COMMON_SOURCES}
# )
# target_include_directories(so_project_candidate PRIVATE include)
# target_compile_features(so_project_candidate PRIVATE cxx_std_17)