cmake_minimum_required(VERSION 3.6)
project(SimpleRPC)

set(CMAKE_CXX_STANDARD 14)

include_directories(
        ${CMAKE_SOURCE_DIR}/boost_lite
        ${CMAKE_SOURCE_DIR}/rpc/include)

set(SIMPLE_RPC
        rpc/include/Exceptions.h
        rpc/include/TypeTraits.h
        rpc/include/Serializable.h
        rpc/src/Serializable.cpp
        rpc/src/TypeTraits.cpp)

add_executable(SimpleRPC ${SIMPLE_RPC} main.cpp)
target_link_libraries(SimpleRPC msgpackc)
