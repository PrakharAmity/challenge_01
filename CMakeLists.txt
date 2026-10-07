cmake_minimum_required(VERSION 3.16)
project(cricpulse LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(cric_core
  src/cric.cpp
  src/partnership_reachability.cpp
  src/best_six_over.cpp
  src/rolling_rate.cpp
  src/strongest_chain.cpp
  src/player_access.cpp
  src/poll_limiter.cpp
)
target_include_directories(cric_core PUBLIC include)
add_executable(server src/main.cpp)
target_link_libraries(server PRIVATE cric_core)
add_executable(challenge_tests tests/challenge_tests.cpp)
target_link_libraries(challenge_tests PRIVATE cric_core)
