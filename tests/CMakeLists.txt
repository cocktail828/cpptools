function(add_gtest_target target_name source_file)
    add_executable(${target_name} ${source_file})
    target_link_libraries(${target_name}
        PRIVATE
        GTest::GTest
        GTest::Main
        Threads::Threads
    )
    
    gtest_discover_tests(${target_name})
endfunction()

add_gtest_target(uri_test uri_test.cpp)
add_gtest_target(scope_guard_test scope_guard_test.cpp)
add_gtest_target(hardware_test hardware_test.cpp)
if (ENABLE_TSS2)
    target_compile_definitions(hardware_test PRIVATE ENABLE_TSS2)
    target_link_libraries(hardware_test PRIVATE tss2-esys)
endif()