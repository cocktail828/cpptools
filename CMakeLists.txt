cmake_minimum_required(VERSION 3.16)
project(cpptools)
set(CMAKE_CXX_STANDARD 20)

# Set default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

# enable TSS2 support
option(ENABLE_TSS2 "Enable TSS2 support" ON)

# Add debug options
add_compile_options(
    -g                      # Generate debug information
    -O0                     # Disable optimization, helpful for debugging
    -Wall                   # Enable all warnings
    -Wextra                 # Enable extra warnings
    -Wpedantic              # Enable strict standard warnings
    -fno-omit-frame-pointer # Keep frame pointer, helpful for debugging
)

include(CTest)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/net)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/utilities)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/slog)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/hardware)

enable_testing()
add_subdirectory(tests)
