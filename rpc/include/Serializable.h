/* Base class of any serializable objects */

#ifndef SIMPLERPC_SERIALIZABLE_H
#define SIMPLERPC_SERIALIZABLE_H

#include <mutex>
#include <unordered_set>

#include <cxxabi.h>
#include <boost/preprocessor.hpp>

#include "Inspector.h"

namespace SimpleRPC
{
typedef Internal::Field     Field;
typedef Internal::Method    Method;
typedef Internal::Variant   Variant;
typedef Internal::Registry  Registry;

class Serializable
{
    std::string _name;
    std::unordered_set<std::string> _names;

private:
    mutable std::mutex _mutex;
    mutable const Internal::Registry::Meta *_meta;

public:
    typedef Internal::Registry::Meta Meta;

protected:
    explicit Serializable() : _meta(nullptr) {}

public:
    bool isSet(const std::string &name) const { return _names.find(name) != _names.end(); }

public:
    const Meta &meta(void) const;
    const std::string &name(void) const { return _name; }

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

#define __SRPC_MEMBER_DECL_VAR(type, elem)              BOOST_PP_SEQ_ELEM(2, elem);
#define __SRPC_MEMBER_REFL_VAR(type, elem)              ::SimpleRPC::Internal::Descriptor<type>::MemberData(__SRPC_STRING(BOOST_PP_SEQ_ELEM(1, elem)), static_cast<type *>(nullptr)->BOOST_PP_SEQ_ELEM(1, elem)),

#define defineClass(type, ...)                                                                                                          \
    struct type final : public ::SimpleRPC::Serializable                                                                                \
    {                                                                                                                                   \
        type() { ::SimpleRPC::Serializable::setName(typeid(type).name()); }                                                             \
        BOOST_PP_SEQ_FOR_EACH(__SRPC_MEMBER_DECL, type, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))                                          \
    };                                                                                                                                  \
                                                                                                                                        \
    static ::SimpleRPC::Internal::Descriptor<type> __SimpleRPC_ ## type ## _Descriptor_DO_NOT_TOUCH_THIS_VARIABLE__ [[gnu::unused]] ({  \
        BOOST_PP_SEQ_FOR_EACH(__SRPC_MEMBER_REFL, type, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))                                          \
    });

#define __SRPC_MEMBER_DECL_FUNC(type, elem)             BOOST_PP_SEQ_ELEM(1, elem) BOOST_PP_SEQ_ELEM(2, elem) (BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_ELEM(3, elem)));
#define __SRPC_MEMBER_REFL_FUNC(type, elem)             ::SimpleRPC::Internal::Descriptor<type>::MemberData(static_cast<BOOST_PP_SEQ_ELEM(1, elem) (type::*)(BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_ELEM(3, elem)))>(&type::BOOST_PP_SEQ_ELEM(2, elem))),

#define defineField(type, name)                         (VAR)(name)(type name)
#define declareMethod(ret, name, ...)                   (FUNC)(ret)(name)(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#endif /* SIMPLERPC_SERIALIZABLE_H */
