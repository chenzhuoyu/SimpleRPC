cmake_minimum_required(VERSION 3.6)
project(SimpleRPC)

set(CMAKE_CXX_STANDARD 14)
add_definitions(-D_BSD_SOURCE)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    include_directories(/usr/local/include)
endif()

include_directories(${CMAKE_SOURCE_DIR}/rpc/include)

set(SIMPLE_RPC
        rpc/include/backend/Backend.h
        rpc/include/backend/MessagePackBackend.h
        rpc/include/network/CallSite.h
        rpc/include/network/InvokeProxy.h
        rpc/include/network/LocalCallSite.h
        rpc/include/ByteSeq.h
        rpc/include/Exceptions.h
        rpc/include/Functional.h
        rpc/include/Inspector.h
        rpc/include/Registry.h
        rpc/include/SimpleRPC.h
        rpc/include/TypeInfo.h
        rpc/include/TypeWrapper.h
        rpc/include/Variant.h
        rpc/src/backend/MessagePackBackend.cpp
        rpc/src/network/LocalCallSite.cpp
        rpc/src/ByteSeq.cpp
        rpc/src/Registry.cpp)

add_executable(SimpleRPC ${SIMPLE_RPC} main.cpp)
target_link_libraries(SimpleRPC)
