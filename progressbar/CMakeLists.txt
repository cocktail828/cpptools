cmake_minimum_required(VERSION 3.10)
project(progressbar VERSION 1.0.0 LANGUAGES CXX)

add_library(progressbar INTERFACE)
target_include_directories(progressbar INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(progressbar INTERFACE cxx_std_11)

find_package(Threads REQUIRED)
target_link_libraries(progressbar INTERFACE Threads::Threads)

option(PROGRESSBAR_BUILD_EXAMPLES "Build examples" ON)
if(PROGRESSBAR_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

option(PROGRESSBAR_BUILD_TESTS "Build tests" OFF)
if(PROGRESSBAR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
