cmake_minimum_required(VERSION 3.15)
project(Project3 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS -pthread)
add_executable(Project3 main.c)