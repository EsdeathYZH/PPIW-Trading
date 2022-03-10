set(CMAKE_CXX_COMPILER h5c++)

project (trader)

## CMake version
cmake_minimum_required(VERSION 2.8)

## Use C++17 features
add_definitions(-std=c++17)

## Set dependencies
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -fopenmp -Wall")

## Set include paths
include_directories(src)

## Source code
file(GLOB SOURCES "src/trader.cpp")
add_executable(trader ${SOURCES})

file(GLOB EXCHANGE_SOURCES "src/exchange.cpp")
add_executable(exchange ${EXCHANGE_SOURCES})

