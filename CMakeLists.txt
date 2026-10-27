cmake_minimum_required(VERSION 3.14)
project(fpga_aviation_hud VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Suppress the FetchContent timestamp warning
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# 1. Require SDL2 (Will fail loudly if pacman install didn't work)
find_package(SDL2 CONFIG REQUIRED)

# 2. SDL2 Graphical HUD Application (Bypassing static library)
add_executable(hud_app
    src/main.cpp
    src/hud_renderer.cpp
    src/telemetry_parser.cpp
)
target_include_directories(hud_app PRIVATE include)
target_link_libraries(hud_app PRIVATE SDL2::SDL2main SDL2::SDL2)

# 3. Fetch GoogleTest
enable_testing()
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 4. GoogleTest Unit Test Suite (Bypassing static library)
add_executable(hud_tests
    tests/test_telemetry.cpp
    src/telemetry_parser.cpp
)
target_include_directories(hud_tests PRIVATE include)
target_link_libraries(hud_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(hud_tests)