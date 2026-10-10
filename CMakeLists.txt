cmake_minimum_required(VERSION 3.20)
project(cricpulse LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(cric_core
    src/analytics.cpp
    src/run_rate.cpp
    src/auth.cpp
    src/rate_limiter.cpp
    src/partnership_routes.cpp
    src/partnership_chain.cpp
    src/match_data.cpp
)

target_include_directories(cric_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Primary server target: server
add_executable(server src/main.cpp)
target_link_libraries(server PRIVATE
    cric_core
    Threads::Threads
)
if(WIN32)
    target_link_libraries(server PRIVATE ws2_32 crypt32)
endif()

# Server alias target
add_executable(cricpulse_server src/main.cpp)
target_link_libraries(cricpulse_server PRIVATE
    cric_core
    Threads::Threads
)
if(WIN32)
    target_link_libraries(cricpulse_server PRIVATE ws2_32 crypt32)
endif()

# Primary test target: challenge_tests
add_executable(challenge_tests tests/challenge_tests.cpp)
target_link_libraries(challenge_tests PRIVATE
    cric_core
    Threads::Threads
)
if(WIN32)
    target_link_libraries(challenge_tests PRIVATE ws2_32 crypt32)
endif()
