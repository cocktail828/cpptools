add_executable(test_bar test_bar.cpp)
target_link_libraries(test_bar PRIVATE progressbar)
add_test(NAME test_bar COMMAND test_bar)
