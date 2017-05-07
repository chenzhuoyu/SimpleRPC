/* Functional utils */

#ifndef SIMPLERPC_FUNCTIONAL_H
#define SIMPLERPC_FUNCTIONAL_H

#include <tuple>
#include <utility>
#include <functional>
#include <type_traits>

#pragma ide diagnostic push
#pragma ide diagnostic ignored "Simplifiable statement"

namespace SimpleRPC
{
namespace Internal
{
namespace Functional
{
namespace Implementation
{
template <typename T, typename Method, class ArgsTuple, size_t ... Index>
constexpr auto applyImpl(T *self, Method &&method, ArgsTuple &&args, std::index_sequence<Index ...>)
{
    /* tuple expansion */
    return (self->*method)(std::get<Index>(std::forward<ArgsTuple>(args)) ...);
}
}

/** compile-time maximum **/

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

/** self-implemented and simplified version of `std::apply()` (since C++/17) **/

template <typename T, typename F, typename Tuple>
constexpr auto apply(T *self, F &&f, Tuple &&t)
{
    /* build an index sequence for tuple expansion */
    return Implementation::applyImpl(
        self,
        std::forward<F>(f),
        std::forward<Tuple>(t),
        std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>()
    );
}
}
}
}

#pragma ide diagnostic pop

#endif /* SIMPLERPC_FUNCTIONAL_H */
