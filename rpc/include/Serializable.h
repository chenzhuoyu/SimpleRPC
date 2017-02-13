/* Base class of any serializable objects */

#ifndef SIMPLERPC_SERIALIZABLE_H
#define SIMPLERPC_SERIALIZABLE_H

#include <sys/types.h>
#include <boost/preprocessor.hpp>

#include "TypeTraits.h"

namespace SimpleRPC
{
class Serializable
{
    std::string _name;
    std::unordered_set<std::string> _names;

private:
    mutable std::mutex _mutex;
    mutable Registry::Meta *_meta;

protected:
    explicit Serializable() : _meta(nullptr) {}

public:
    bool isSet(const std::string &name) const { return _names.find(name) != _names.end(); }

public:
    const std::string &name(void) const { return _name; }
    const Registry::Meta &meta(void) const;

protected:
    void setName(const std::string &name) { _name = name; }
    void setField(const std::string &name) { _names.insert(name); }

public:
    std::string readableName(void) const
    {
        int status;
        char *demangled = abi::__cxa_demangle(_name.c_str(), nullptr, nullptr, &status);
        std::string result = demangled ? std::string(demangled) : _name;

        free(demangled);
        return result;
    }
};
}

#define __SRPC_MACRO(_1, _2, _3, macro, ...)            macro

#define __SRPC_STRING_IM(x)                             #x
#define __SRPC_STRING(x)                                __SRPC_STRING_IM(x)

#define __SRPC_MEMBER_DECL(r, data, elem)               BOOST_PP_CAT(__SRPC_MEMBER_DECL_, BOOST_PP_SEQ_ELEM(0, elem))(data, elem)
#define __SRPC_MEMBER_REFL(r, data, elem)               BOOST_PP_CAT(__SRPC_MEMBER_REFL_, BOOST_PP_SEQ_ELEM(0, elem))(data, elem)

#define __SRPC_MEMBER_DECL_VAR(type, elem)              BOOST_PP_SEQ_ELEM(3, elem);
#define __SRPC_MEMBER_REFL_VAR(type, elem)              BOOST_PP_CAT(__SRPC_MEMBER_REFL_VAR_, BOOST_PP_SEQ_ELEM(1, elem))(type, BOOST_PP_SEQ_ELEM(2, elem))

#define __SRPC_MEMBER_REFL_VAR_REQ(type, field)         ::SimpleRPC::Descriptor<type>::MemberData(__SRPC_STRING(field), ::SimpleRPC::resolve(((type *)nullptr)->field), (size_t)(&(((type *)nullptr)->field)), true),
#define __SRPC_MEMBER_REFL_VAR_OPT(type, field)         ::SimpleRPC::Descriptor<type>::MemberData(__SRPC_STRING(field), ::SimpleRPC::resolve(((type *)nullptr)->field), (size_t)(&(((type *)nullptr)->field)), false),

#define defineClass(type, ...)                                                                                  \
    struct type : public ::SimpleRPC::Serializable                                                              \
    {                                                                                                           \
        type() { ::SimpleRPC::Serializable::setName(typeid(type).name()); }                                     \
        BOOST_PP_SEQ_FOR_EACH(__SRPC_MEMBER_DECL, type, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))                  \
    };                                                                                                          \
                                                                                                                \
    static ::SimpleRPC::Descriptor<type> __SimpleRPC_ ## type ## _Descriptor_DO_NOT_TOUCH_THIS_VARIABLE__({     \
        BOOST_PP_SEQ_FOR_EACH(__SRPC_MEMBER_REFL, type, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))                  \
    });

#define __SRPC_DEFINE_FIELD_REQ(type, name)             (VAR)(REQ)(name)(type name)
#define __SRPC_DEFINE_FIELD_OPT(type, name, value)      (VAR)(OPT)(name)(type name = value)

#define defineField(...)                                \
    __SRPC_MACRO(                                       \
        __VA_ARGS__,                                    \
        __SRPC_DEFINE_FIELD_OPT,                        \
        __SRPC_DEFINE_FIELD_REQ                         \
    )(__VA_ARGS__)

#define __SRPC_DEFINE_ARG_REQ(type, name)               (REQ)(name)(type name)
#define __SRPC_DEFINE_ARG_OPT(type, name, value)        (OPT)(name)(type name = value)

#define defineArg(...)                                  \
    __SRPC_MACRO(                                       \
        __VA_ARGS__,                                    \
        __SRPC_DEFINE_ARG_OPT,                          \
        __SRPC_DEFINE_ARG_REQ                           \
    )(__VA_ARGS__)

#define __SRPC_EXPAND_ARG(s, data, elem)                BOOST_PP_SEQ_ELEM(2, elem)
#define __SRPC_MEMBER_DECL_FUNC(type, elem)             BOOST_PP_SEQ_ELEM(1, elem) BOOST_PP_SEQ_ELEM(2, elem) (BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(__SRPC_EXPAND_ARG, type, BOOST_PP_SEQ_ELEM(3, elem))));
#define __SRPC_MEMBER_REFL_FUNC(type, elem)

#define defineMethod(ret, name, ...)                    (FUNC)(ret)(name)(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#endif /* SIMPLERPC_SERIALIZABLE_H */
