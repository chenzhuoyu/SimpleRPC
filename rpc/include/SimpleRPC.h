/* Base class of any serializable objects */

#ifndef SIMPLERPC_SERIALIZABLE_H
#define SIMPLERPC_SERIALIZABLE_H

#include <boost/preprocessor.hpp>

#include "Registry.h"
#include "Inspector.h"

namespace SimpleRPC
{
typedef Internal::Field         Field;
typedef Internal::Method        Method;
typedef Internal::Variant       Variant;
typedef Internal::Registry      Registry;
typedef Internal::Serializable  Serializable;

template <typename T>
struct SerializableWrapper : public Serializable
{
    explicit SerializableWrapper() :
        Serializable(Registry::findClass(Internal::TypeItem<T>::type().toSignature())) {}
};
}

#define __SRPC_MEMBER_DECL(r, data, elem)               BOOST_PP_CAT(__SRPC_MEMBER_DECL_, BOOST_PP_SEQ_ELEM(0, elem))(data, elem)
#define __SRPC_MEMBER_REFL(r, data, elem)               BOOST_PP_CAT(__SRPC_MEMBER_REFL_, BOOST_PP_SEQ_ELEM(0, elem))(data, elem)

#define defineClass(type, ...)                                                                                                          \
    struct type final : public ::SimpleRPC::SerializableWrapper<type>                                                                   \
    {                                                                                                                                   \
        type() {}                                                                                                                       \
        BOOST_PP_SEQ_FOR_EACH(__SRPC_MEMBER_DECL, type, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))                                          \
    };                                                                                                                                  \
                                                                                                                                        \
    static ::SimpleRPC::Internal::Descriptor<type> __SimpleRPC_Descriptor_ ## type ## _DO_NOT_TOUCH_THIS_VARIABLE__ [[gnu::unused]] ({  \
        BOOST_PP_SEQ_FOR_EACH(__SRPC_MEMBER_REFL, type, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))                                          \
    });

#define __SRPC_MEMBER_DECL_LIT(type, elem)              BOOST_PP_SEQ_ELEM(1, elem)
#define __SRPC_MEMBER_REFL_LIT(type, elem)

#define defineLiteral(literal)                          (LIT)(literal)

#define __SRPC_FIELD_SIG_CAST(type, elem)               static_cast<type *>(nullptr)->BOOST_PP_SEQ_ELEM(1, elem)

#define __SRPC_MEMBER_DECL_VAR(type, elem)              BOOST_PP_SEQ_ELEM(2, elem);
#define __SRPC_MEMBER_REFL_VAR(type, elem)              ::SimpleRPC::Internal::Descriptor<type>::MemberData(BOOST_PP_STRINGIZE(BOOST_PP_SEQ_ELEM(1, elem)), __SRPC_FIELD_SIG_CAST(type, elem)),

#define defineField(type, name)                         (VAR)(name)(type name)

#define __SRPC_METHOD_ARG_LIST(elem)                    BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_ELEM(3, elem))
#define __SRPC_METHOD_SIG_TYPE(type, elem)              BOOST_PP_SEQ_ELEM(1, elem) (type::*)(__SRPC_METHOD_ARG_LIST(elem))
#define __SRPC_METHOD_SIG_CAST(type, elem)              static_cast<__SRPC_METHOD_SIG_TYPE(type, elem)>(&type::BOOST_PP_SEQ_ELEM(2, elem))

#define __SRPC_MEMBER_DECL_FUNC(type, elem)             BOOST_PP_SEQ_ELEM(1, elem) BOOST_PP_SEQ_ELEM(2, elem) (__SRPC_METHOD_ARG_LIST(elem));
#define __SRPC_MEMBER_REFL_FUNC(type, elem)             ::SimpleRPC::Internal::Descriptor<type>::MemberData(BOOST_PP_STRINGIZE(BOOST_PP_SEQ_ELEM(2, elem)), __SRPC_METHOD_SIG_CAST(type, elem)),

#define declareMethod(ret, name, ...)                   (FUNC)(ret)(name)(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#endif /* SIMPLERPC_SERIALIZABLE_H */
