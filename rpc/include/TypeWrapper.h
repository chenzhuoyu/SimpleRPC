/* Wrapper type for lvalue reference */

#ifndef SIMPLERPC_TYPEWRAPPER_H
#define SIMPLERPC_TYPEWRAPPER_H

#include <memory>
#include <type_traits>

namespace SimpleRPC
{
namespace Internal
{
template <typename T>
class TypeWrapper final
{
    std::shared_ptr<T> _value;

public:
    typedef T Type;
    static_assert(!std::is_pointer<T>::value, "`T` must not be pointers");
    static_assert(!std::is_reference<T>::value, "`T` must not be references");

public:
    T &operator*() const { return *_value; }
    TypeWrapper(std::shared_ptr<T> &&value) : _value(std::move(value)) {}

};

template <typename T> struct IsTypeWrapper: public std::false_type {};
template <typename T> struct IsTypeWrapper<TypeWrapper<T>> : public std::true_type {};
}
}

#endif /* SIMPLERPC_TYPEWRAPPER_H */
