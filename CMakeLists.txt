cmake_minimum_required(VERSION 3.16)
project(uri_parser)
set(CMAKE_CXX_STANDARD 20)

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

add_library(uri_parser INTERFACE)
target_include_directories(uri_parser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(uri_parser_tests tests/uri_parser_gtest.cpp)
target_link_libraries(uri_parser_tests PRIVATE uri_parser ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} pthread)

# Explicitly enable debug options for the test executable
target_compile_options(uri_parser_tests PRIVATE -g -O0)

add_test(NAME uri_parser_tests COMMAND uri_parser_tests)

# Add a custom target to run the tests with gdb
add_custom_target(run_debug
    COMMAND gdb --args $<TARGET_FILE:uri_parser_tests>
    DEPENDS uri_parser_tests
    COMMENT "Run gdb debugger on uri_parser_tests"
)