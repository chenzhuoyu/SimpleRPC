/* Base class of any serializable objects */

#ifndef SIMPLERPC_SERIALIZABLE_H
#define SIMPLERPC_SERIALIZABLE_H

#include <utility>
#include <boost/preprocessor.hpp>

#include "Registry.h"
#include "Inspector.h"
#include "Exceptions.h"

#include "network/CallSite.h"
#include "network/InvokeProxy.h"

namespace SimpleRPC
{
template <typename T>
static Registry::MetaClass metaClassOf(void)
{
    /* find meta data in class registry */
    static Registry::MetaClass meta = Registry::findClass(Internal::TypeItem<T>::type().toSignature());
    return meta;
}

template <typename T>
struct SerializableWrapper : public Serializable
{
    /* automatically initialize the class meta data */
    explicit SerializableWrapper() : Serializable(metaClassOf<T>()) {}
};
}

#define __SRPC_PROXY_DECL(r, data, elem)                BOOST_PP_CAT(__SRPC_PROXY_DECL_, BOOST_PP_SEQ_ELEM(0, elem))(data, elem)
#define __SRPC_MEMBER_DECL(r, data, elem)               BOOST_PP_CAT(__SRPC_MEMBER_DECL_, BOOST_PP_SEQ_ELEM(0, elem))(data, elem)
#define __SRPC_MEMBER_REFL(r, data, elem)               BOOST_PP_CAT(__SRPC_MEMBER_REFL_, BOOST_PP_SEQ_ELEM(0, elem))(data, elem)

#define defineClass(type, ...)                                                                                                  \
    struct type final : public ::SimpleRPC::SerializableWrapper<type>                                                           \
    {                                                                                                                           \
        type() {}                                                                                                               \
        BOOST_PP_SEQ_FOR_EACH(__SRPC_MEMBER_DECL, type, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))                                  \
                                                                                                                                \
        struct Proxy : public ::SimpleRPC::Network::InvokeProxyAdapter<type>                                                    \
        {                                                                                                                       \
            using ::SimpleRPC::Network::InvokeProxyAdapter<type>::InvokeProxyAdapter;                                           \
            BOOST_PP_SEQ_FOR_EACH(__SRPC_PROXY_DECL, type, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))                               \
        };                                                                                                                      \
    };                                                                                                                          \
                                                                                                                                \
    static ::SimpleRPC::Descriptor<type> __SimpleRPC_Descriptor_ ## type ## _DO_NOT_TOUCH_THIS_VARIABLE__ [[gnu::unused]] ({    \
        BOOST_PP_SEQ_FOR_EACH(__SRPC_MEMBER_REFL, type, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))                                  \
    });

#define __SRPC_METHOD_ARG_LIST(elem)                    BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_ELEM(3, elem))
#define __SRPC_METHOD_SIG_TYPE(type, elem)              BOOST_PP_SEQ_ELEM(1, elem) (type::*)(__SRPC_METHOD_ARG_LIST(elem))

#define __SRPC_FIELD_SIG_CAST(type, elem)               static_cast<type *>(nullptr)->BOOST_PP_SEQ_ELEM(1, elem)
#define __SRPC_METHOD_SIG_CAST(type, elem)              static_cast<__SRPC_METHOD_SIG_TYPE(type, elem)>(&type::BOOST_PP_SEQ_ELEM(2, elem))

#define __SRPC_PROXY_ARG_ITEM(i, name, type)            BOOST_PP_IF(BOOST_PP_IS_EMPTY(type), BOOST_PP_EMPTY(), type BOOST_PP_CAT(name, BOOST_PP_SUB(i, 2)))
#define __SRPC_PROXY_ARG_LIST(elem)                     BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(__SRPC_PROXY_ARG_ITEM, _, BOOST_PP_SEQ_ELEM(3, elem)))

#define __SRPC_PROXY_CALL_ITEM(i, name, type)           BOOST_PP_IF(BOOST_PP_IS_EMPTY(type), BOOST_PP_EMPTY(), std::forward<type>(BOOST_PP_CAT(name, BOOST_PP_SUB(i, 2))))
#define __SRPC_PROXY_CALL_LIST(elem)                    BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(__SRPC_PROXY_CALL_ITEM, _, BOOST_PP_SEQ_ELEM(3, elem)))

#define __SRPC_PROXY_DECL_RAW(type, elem)
#define __SRPC_PROXY_DECL_VAR(type, elem)
#define __SRPC_PROXY_DECL_FUNC(type, elem)                                                                      \
    BOOST_PP_SEQ_ELEM(1, elem) BOOST_PP_SEQ_ELEM(2, elem) (__SRPC_PROXY_ARG_LIST(elem))                         \
    {                                                                                                           \
        return ::SimpleRPC::Network::InvokeProxyAdapter<type>::invoke<BOOST_PP_SEQ_ELEM(1, elem)> BOOST_PP_IF(  \
            BOOST_PP_IS_EMPTY(BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_ELEM(3, elem))),                                   \
            (BOOST_PP_STRINGIZE(BOOST_PP_SEQ_ELEM(2, elem))),                                                   \
            (BOOST_PP_STRINGIZE(BOOST_PP_SEQ_ELEM(2, elem)), __SRPC_PROXY_CALL_LIST(elem))                      \
        );                                                                                                      \
    }

#define __SRPC_MEMBER_DECL_RAW(type, elem)              BOOST_PP_SEQ_ELEM(1, elem)
#define __SRPC_MEMBER_DECL_VAR(type, elem)              BOOST_PP_SEQ_ELEM(2, elem);
#define __SRPC_MEMBER_DECL_FUNC(type, elem)             BOOST_PP_SEQ_ELEM(1, elem) BOOST_PP_SEQ_ELEM(2, elem) (__SRPC_METHOD_ARG_LIST(elem));

#define __SRPC_MEMBER_REFL_RAW(type, elem)
#define __SRPC_MEMBER_REFL_VAR(type, elem)              ::SimpleRPC::Descriptor<type>::MemberData(BOOST_PP_STRINGIZE(BOOST_PP_SEQ_ELEM(1, elem)), __SRPC_FIELD_SIG_CAST(type, elem)),
#define __SRPC_MEMBER_REFL_FUNC(type, elem)             ::SimpleRPC::Descriptor<type>::MemberData(BOOST_PP_STRINGIZE(BOOST_PP_SEQ_ELEM(2, elem)), __SRPC_METHOD_SIG_CAST(type, elem)),

#define defineRaw(stmt)                                 (RAW)(stmt)
#define defineField(type, name)                         (VAR)(name)(type name = type())
#define declareMethod(ret, name, args)                  (FUNC)(ret)(name)(BOOST_PP_TUPLE_TO_SEQ(args))

#endif /* SIMPLERPC_SERIALIZABLE_H */
