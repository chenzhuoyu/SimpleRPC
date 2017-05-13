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
constexpr auto invokeByTuple(T *self, F &&f, ArgsTuple &&args, std::index_sequence<Index ...>)
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

template <typename V, typename R, typename T, typename P, typename ... Args>
struct MetaFunction
{
    static V invoke(T *self, R (T::*&&method)(Args ...), P &tuple)
    {
        /* non-void return type version */
        return V(Implementation::invokeByTuple(self, std::move(method), tuple, std::index_sequence_for<Args ...>()));
    }
};

template <typename V, typename T, typename P, typename ... Args>
struct MetaFunction<V, void, T, P, Args ...>
{
    static V invoke(T *self, void (T::*&&method)(Args ...), P &tuple)
    {
        /* void return type specification */
        Implementation::invokeByTuple(self, std::move(method), tuple, std::index_sequence_for<Args ...>());
        return V();
    }
};
}
}
}

#endif /* SIMPLERPC_FUNCTIONAL_H */
