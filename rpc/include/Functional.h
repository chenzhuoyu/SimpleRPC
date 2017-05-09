/* Functional utils */

#ifndef SIMPLERPC_FUNCTIONAL_H
#define SIMPLERPC_FUNCTIONAL_H

#include <tuple>
#include <utility>
#include <functional>
#include <type_traits>

#include "TypeWrapper.h"

namespace SimpleRPC
{
namespace Internal
{
namespace Functional
{
namespace Implementation
{
template <typename T>
struct Expander
{
    typedef T Type;
    static constexpr T &expand(T &&value) { return value; }
};

template <typename T>
struct Expander<TypeWrapper<T> &>
{
    typedef T Type;
    static constexpr T &expand(TypeWrapper<T> &value) { return *value; }
};

template <typename T>
constexpr typename Expander<T>::Type &expand(T &&value)
{
    return Expander<T>::expand(std::forward<T>(value));
}

template <typename T, typename F, class ArgsTuple, size_t ... Index>
constexpr auto invokeMethodByTuple(T *self, F &&f, ArgsTuple &&args, std::index_sequence<Index ...>)
{
    return (self->*f)(expand(std::get<Index>(std::forward<ArgsTuple>(args))) ...);
}
}

template <typename T>
constexpr T &&max(T &&a, T &&b)
{
    return a > b ? std::forward<T>(a) : std::forward<T>(b);
}

template <typename T, typename ... Args>
constexpr T &&max(T &&a, Args && ... args)
{
    return max(std::forward<T>(a), max(std::forward<Args>(args) ...));
}

template <typename T, typename F, typename Tuple>
constexpr auto invokeMethod(T *self, F &&f, Tuple &&args)
{
    return Implementation::invokeMethodByTuple(
        self,
        std::forward<F>(f),
        std::forward<Tuple>(args),
        std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>()
    );
}
}
}
}

#endif /* SIMPLERPC_FUNCTIONAL_H */
