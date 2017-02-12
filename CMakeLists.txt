cmake_minimum_required(VERSION 3.6)
project(SimpleRPC)

set(CMAKE_CXX_STANDARD 14)

include_directories(
        ${CMAKE_SOURCE_DIR}/boost_lite
        ${CMAKE_SOURCE_DIR}/include)

set(SOURCE_FILES main.cpp rpc/Registry.cpp rpc/Registry.h rpc/TypeTraits.h rpc/Exceptions.h)

add_executable(SimpleRPC ${SOURCE_FILES})
