add_executable(docker_pull docker_pull.cpp)
target_link_libraries(docker_pull PRIVATE progressbar)

add_executable(apt_style apt_style.cpp)
target_link_libraries(apt_style PRIVATE progressbar)
