cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Test)

add_subdirectory(server)
add_executable(test main.cpp)
target_link_libraries(test PUBLIC asyncserver)
