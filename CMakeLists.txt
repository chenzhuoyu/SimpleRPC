cmake_minimum_required(VERSION 3.6)
project(SimpleRPC)

set(CMAKE_CXX_STANDARD 14)
add_definitions(-D_BSD_SOURCE)

include_directories(
        ${CMAKE_SOURCE_DIR}/rpc/include)

set(SIMPLE_RPC
        rpc/include/Exceptions.h
        rpc/include/Functional.h
        rpc/include/Inspector.h
        rpc/include/Serializable.h
        rpc/include/TypeInfo.h
        rpc/include/Variant.h
        rpc/src/Inspector.cpp)

add_executable(SimpleRPC ${SIMPLE_RPC} main.cpp)
target_link_libraries(SimpleRPC msgpackc)
