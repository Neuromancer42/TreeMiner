cmake_minimum_required(VERSION 3.12)
project(TreeMiner)

set(CMAKE_CXX_STANDARD 14)

if(DEBUG)
    add_definitions(-DDEBUG=1)
endif(DEBUG)

add_executable(TreeMiner main.cpp)
