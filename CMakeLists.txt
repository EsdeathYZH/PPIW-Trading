set(CMAKE_CXX_COMPILER h5c++)

project (trader)

## CMake version
cmake_minimum_required(VERSION 3.2)

## Use C++17 features
add_definitions(-std=c++17)

## Set dependencies
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -fopenmp -Wall")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -fopenmp -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable -Wno-unused-function -Wno-parentheses")

## Set include paths
include_directories(deps/boost_1_67_0-install/include)
include_directories(deps/zeromq-4.0.5-install/include)
include_directories(src)

## Set link library paths
link_directories(deps/boost_1_67_0-install/lib)
link_directories(deps/zeromq-4.0.5-install/lib)

## Set boost
set(BOOST_LIB_DIR "${CMAKE_SOURCE_DIR}/deps/boost_1_67_0-install/lib")
set(BOOST_LIBS ${BOOST_LIBS} ${BOOST_LIB_DIR}/libboost_program_options.so ${BOOST_LIB_DIR}/libboost_thread.so ${BOOST_LIB_DIR}/libboost_system.so)

## Source code
file(GLOB_RECURSE TRADER_SOURCES
    "src/common/*.cpp"
    "src/trader/*.cpp"
    "src/network/*.cpp"
    "src/utils/*.cpp"
    "src/trader.cpp"
)
add_executable(trader ${TRADER_SOURCES})
target_link_libraries(trader ${CMAKE_SOURCE_DIR}/deps/zeromq-4.0.5-install/lib/libzmq.so ${BOOST_LIBS})

file(GLOB_RECURSE EXCHANGE_SOURCES
    "src/common/*.cpp"
    "src/exchange/*.cpp"
    "src/network/*.cpp"
    "src/utils/*.cpp"
    "src/exchange.cpp"
)
add_executable(exchange ${EXCHANGE_SOURCES})
target_link_libraries(exchange ${CMAKE_SOURCE_DIR}/deps/zeromq-4.0.5-install/lib/libzmq.so ${BOOST_LIBS})

# tools: output_diff
add_executable(
    output_diff
    "src/output_diff.cpp"
    "src/common/config.cpp")

# tools: query_order
add_executable(
    query_order
    "src/query_order.cpp"
    "src/common/config.cpp"
)

