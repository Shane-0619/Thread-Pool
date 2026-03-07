cmake_minimum_required(VERSION 3.10)

project(ThreadPool)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)

add_executable(main
    src/main.cpp
    src/threadpool.cpp
)

target_link_libraries(main pthread)