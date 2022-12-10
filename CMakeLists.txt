cmake_minimum_required(VERSION 3.23)
project(online_traffic_sim)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../emsdk/upstream/emscripten/system/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../emsdk/upstream/emscripten/cache/ports/boost_headers)

add_executable(online_traffic_sim
        main.cpp
        json.hpp)
