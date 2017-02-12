/* Base class of any serializable objects */

#ifndef SIMPLERPC_SERIALIZABLE_H
#define SIMPLERPC_SERIALIZABLE_H

#include <sys/types.h>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/to_tuple.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#include "TypeTraits.h"
#include "Exceptions.h"

namespace SimpleRPC
{

}

#define __SRPC_EXTRACT_IM(...)                          __VA_ARGS__
#define __SRPC_EXTRACT_I(...)                           __SRPC_EXTRACT_IM __VA_ARGS__
#define __SRPC_EXTRACT(seq)                             __SRPC_EXTRACT_I(BOOST_PP_SEQ_TO_TUPLE(seq))

#define __SRPC_STRING_IM(x)                             #x
#define __SRPC_STRING(x)                                __SRPC_STRING_IM(x)

#define __SRPC_MACRO(_1, _2, _3, macro, ...)            macro

#define __SRPC_MEMBER_DECL(r, data, elem)               BOOST_PP_CAT(__SRPC_MEMBER_DECL_, BOOST_PP_SEQ_ELEM(0, elem))(data, elem)
#define __SRPC_MEMBER_REFL(r, data, elem)               BOOST_PP_CAT(__SRPC_MEMBER_REFL_, BOOST_PP_SEQ_ELEM(0, elem))(data, elem)

#define __SRPC_MEMBER_DECL_VAR(name, elem)              BOOST_PP_SEQ_ELEM(3, elem);
#define __SRPC_MEMBER_REFL_VAR(name, elem)              BOOST_PP_CAT(__SRPC_MEMBER_REFL_VAR_, BOOST_PP_SEQ_ELEM(1, elem))(name, BOOST_PP_SEQ_ELEM(2, elem))

#define __SRPC_MEMBER_REFL_VAR_REQ(type, field)         ::SimpleRPC::Descriptor<type>::FieldInfo(__SRPC_STRING(field), ::SimpleRPC::resolve(((type *)nullptr)->field), (size_t)(&(((type *)nullptr)->field)), true),
#define __SRPC_MEMBER_REFL_VAR_OPT(type, field)         ::SimpleRPC::Descriptor<type>::FieldInfo(__SRPC_STRING(field), ::SimpleRPC::resolve(((type *)nullptr)->field), (size_t)(&(((type *)nullptr)->field)), false),

#define __SRPC_DEFINE_FIELD_OPT_2(type, name)           (VAR)(OPT)(name)(type name)
#define __SRPC_DEFINE_FIELD_OPT_3(type, name, value)    (VAR)(OPT)(name)(type name = value)

#define defineRequiredField(type, name)                 (VAR)(REQ)(name)(type name)
#define defineOptionalField(...)                        \
    __SRPC_MACRO(                                       \
        __VA_ARGS__,                                    \
        __SRPC_DEFINE_FIELD_OPT_3,                      \
        __SRPC_DEFINE_FIELD_OPT_2                       \
    )(__VA_ARGS__)

#define defineClass(Name, ...)                                                                                  \
    struct Name : public ::SimpleRPC::Struct                                                                    \
    {                                                                                                           \
        Name() { ::SimpleRPC::Struct::setName(typeid(Name).name()); }                                           \
        BOOST_PP_SEQ_FOR_EACH(__SRPC_MEMBER_DECL, Name, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))                  \
    };                                                                                                          \
                                                                                                                \
    static ::SimpleRPC::Descriptor<Name> __SimpleRPC_ ## Name ## _Descriptor_DO_NOT_TOUCH_THIS_VARIABLE__({     \
        BOOST_PP_SEQ_FOR_EACH(__SRPC_MEMBER_REFL, Name, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))                  \
    });

#endif /* SIMPLERPC_SERIALIZABLE_H */
