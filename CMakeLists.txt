cmake_minimum_required(VERSION 3.6)
project(monitor)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
add_executable(monitor ${SOURCE_FILES})
target_link_libraries(monitor -lpthread)
