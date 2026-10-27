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

# The MSYS2 SDL2 package exports both its include root and include/SDL2.
# This target includes <SDL.h>, so only the latter is needed. Keeping the
# parent include root in the system path breaks GCC's include_next lookup for
# standard headers such as stdlib.h in this MinGW build.
if(TARGET SDL2::SDL2)
    set_property(TARGET SDL2::SDL2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}")
endif()
if(TARGET SDL2::SDL2main)
    set_property(TARGET SDL2::SDL2main PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}")
endif()

# 2. SDL2 Graphical HUD Application (Bypassing static library)
add_executable(hud_app
    src/main.cpp
    src/hud_renderer.cpp
    src/attitude_smoother.cpp
    src/telemetry_parser.cpp
)
target_include_directories(hud_app PRIVATE include)
target_link_libraries(hud_app PRIVATE SDL2::SDL2main SDL2::SDL2)

# 3. GoogleTest unit tests. Disable with -DBUILD_TESTING=OFF for app-only builds.
include(CTest)
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(hud_tests
        tests/test_telemetry.cpp
        tests/test_attitude_smoother.cpp
        src/telemetry_parser.cpp
        src/attitude_smoother.cpp
    )
    target_include_directories(hud_tests PRIVATE include)
    target_link_libraries(hud_tests PRIVATE GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(hud_tests)
endif()

# 4. FPGA bitstream. Include this in the default build when the complete
# open-source FPGA toolchain is installed; the host HUD alone cannot rebuild
# the VHDL or the pin constraints.
find_program(GHDL_EXECUTABLE NAMES ghdl)
find_program(YOSYS_EXECUTABLE NAMES yosys)
find_program(NEXTPNR_ICE40_EXECUTABLE NAMES nextpnr-ice40)
find_program(ICEPACK_EXECUTABLE NAMES icepack)
find_program(RTL_MAKE_EXECUTABLE NAMES make gmake mingw32-make)

if(GHDL_EXECUTABLE AND YOSYS_EXECUTABLE AND NEXTPNR_ICE40_EXECUTABLE AND
   ICEPACK_EXECUTABLE AND RTL_MAKE_EXECUTABLE)
    add_custom_target(fpga_firmware ALL
        COMMAND "${RTL_MAKE_EXECUTABLE}" -f MakeFile
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rtl"
        BYPRODUCTS "${CMAKE_CURRENT_SOURCE_DIR}/rtl/hud_top.bin"
        COMMENT "Building the iCE40 FPGA bitstream"
        VERBATIM
    )
else()
    message(STATUS "FPGA bitstream target disabled: install GHDL, Yosys, nextpnr-ice40, icepack, and make")
endif()
