cmake_minimum_required(VERSION 3.16)
project(cricpulse LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(cric_core
    src/cric.cpp
    src/partnership_reachability.cpp
    src/best_six_over.cpp
    src/rolling_rate.cpp
    src/strongest_chain.cpp
    src/player_access.cpp
    src/poll_limiter.cpp
)

target_include_directories(cric_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Server executable: cricpulse_server
add_executable(cricpulse_server src/main.cpp)
target_link_libraries(cricpulse_server PRIVATE
    cric_core
    Threads::Threads
)

if(WIN32)
    target_link_libraries(cricpulse_server PRIVATE ws2_32 crypt32)
endif()

# Backward compatible alias/target for server
add_executable(server src/main.cpp)
target_link_libraries(server PRIVATE
    cric_core
    Threads::Threads
)
if(WIN32)
    target_link_libraries(server PRIVATE ws2_32 crypt32)
endif()

# Test runner executables
add_executable(test_runner tests/run_tests.cpp)
target_link_libraries(test_runner PRIVATE
    cric_core
    Threads::Threads
)
if(WIN32)
    target_link_libraries(test_runner PRIVATE ws2_32 crypt32)
endif()

add_executable(challenge_tests tests/challenge_tests.cpp)
target_link_libraries(challenge_tests PRIVATE
    cric_core
    Threads::Threads
)
if(WIN32)
    target_link_libraries(challenge_tests PRIVATE ws2_32 crypt32)
endif()
