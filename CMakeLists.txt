cmake_minimum_required(VERSION 3.6)
project(SimpleRPC)

set(CMAKE_CXX_STANDARD 14)

include_directories(
        ${CMAKE_SOURCE_DIR}/boost_lite
        ${CMAKE_SOURCE_DIR}/rpc/include)

set(BOOST_PP
        boost_lite/boost/preprocessor/arithmetic/dec.hpp
        boost_lite/boost/preprocessor/cat.hpp
        boost_lite/boost/preprocessor/config/config.hpp
        boost_lite/boost/preprocessor/control/expr_iif.hpp
        boost_lite/boost/preprocessor/control/if.hpp
        boost_lite/boost/preprocessor/control/iif.hpp
        boost_lite/boost/preprocessor/debug/error.hpp
        boost_lite/boost/preprocessor/detail/auto_rec.hpp
        boost_lite/boost/preprocessor/facilities/empty.hpp
        boost_lite/boost/preprocessor/facilities/expand.hpp
        boost_lite/boost/preprocessor/facilities/overload.hpp
        boost_lite/boost/preprocessor/logical/bool.hpp
        boost_lite/boost/preprocessor/logical/compl.hpp
        boost_lite/boost/preprocessor/repetition/detail/for.hpp
        boost_lite/boost/preprocessor/repetition/for.hpp
        boost_lite/boost/preprocessor/seq/detail/is_empty.hpp
        boost_lite/boost/preprocessor/seq/elem.hpp
        boost_lite/boost/preprocessor/seq/enum.hpp
        boost_lite/boost/preprocessor/seq/for_each.hpp
        boost_lite/boost/preprocessor/seq/seq.hpp
        boost_lite/boost/preprocessor/seq/size.hpp
        boost_lite/boost/preprocessor/seq/to_tuple.hpp
        boost_lite/boost/preprocessor/tuple/detail/is_single_return.hpp
        boost_lite/boost/preprocessor/tuple/eat.hpp
        boost_lite/boost/preprocessor/tuple/elem.hpp
        boost_lite/boost/preprocessor/tuple/rem.hpp
        boost_lite/boost/preprocessor/tuple/size.hpp
        boost_lite/boost/preprocessor/tuple/to_seq.hpp
        boost_lite/boost/preprocessor/variadic/elem.hpp
        boost_lite/boost/preprocessor/variadic/size.hpp
        boost_lite/boost/preprocessor/variadic/to_seq.hpp)

set(SIMPLE_RPC
        ${BOOST_PP}
        rpc/include/Exceptions.h
        rpc/include/TypeTraits.h
        rpc/include/Serializable.h rpc/src/TypeTraits.cpp)

add_executable(SimpleRPC ${SIMPLE_RPC} main.cpp)
target_link_libraries(SimpleRPC msgpackc)
