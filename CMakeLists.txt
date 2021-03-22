cmake_minimum_required(VERSION 3.10)
project(ThreadPool)

set(CMAKE_CXX_STANDARD 11)

add_executable(ThreadPool example.cpp ThreadPool.hpp)

find_package(Threads REQUIRED)
target_link_libraries(ThreadPool Threads::Threads)

