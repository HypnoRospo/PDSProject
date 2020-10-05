cmake_minimum_required(VERSION 3.17)
project(PDSProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread" )

add_executable(PDSProject main.cpp)