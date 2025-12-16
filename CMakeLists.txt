cmake_minimum_required(VERSION 3.16)
project(cpptools)
set(CMAKE_CXX_STANDARD 17)

# Set default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

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
include_directories(${GTEST_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/net)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/utilities)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/slog)

add_executable(uri_tests tests/uri_gtest.cpp)
target_link_libraries(uri_tests PRIVATE ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} pthread)

add_executable(scope_guard_tests tests/scope_guard_gtest.cpp)
target_link_libraries(scope_guard_tests PRIVATE ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} pthread)
