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
template <typename T, typename Method, class ArgsTuple, size_t ... I>
constexpr auto applyImpl(T *self, Method &&method, ArgsTuple &&args, std::index_sequence<I ...>)
{
    return (self->*method)(std::get<I>(std::forward<ArgsTuple>(args)) ...);
}
}

template <typename T, typename F, typename Tuple>
constexpr auto apply(T *self, F &&f, Tuple &&t)
{
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
