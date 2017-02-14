/* Functional utils */

#ifndef SIMPLERPC_FUNCTIONAL_H
#define SIMPLERPC_FUNCTIONAL_H

#include <utility>
#include <functional>
#include <type_traits>
#include <sys/types.h>

#pragma ide diagnostic push
#pragma ide diagnostic ignored "Simplifiable statement"

namespace SimpleRPC
{
namespace Functional
{
namespace Internal
{
template <typename T> struct is_reference_wrapper : std::false_type {};
template <typename U> struct is_reference_wrapper<std::reference_wrapper<U>> : std::true_type {};

template <typename T> constexpr bool is_function_v = std::is_function<T>::value;
template <typename T> constexpr bool is_member_pointer_v = std::is_member_pointer<T>::value;
template <typename T> constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;
template <typename U, typename T> constexpr bool is_base_of_v = std::is_base_of<U, T>::value;

template <typename Base, typename T, typename Derived, typename ... Args>
auto invokeImpl(
    T Base::*pmf,
    Derived && ref,
    Args && ... args
) noexcept(noexcept((std::forward<Derived>(ref).*pmf)(std::forward<Args>(args) ...)))
-> std::enable_if_t<
    is_function_v<T> &&
    is_base_of_v<Base, std::decay_t<Derived>>,
    decltype((std::forward<Derived>(ref).*pmf)(std::forward<Args>(args) ...))
>
{
    return (std::forward<Derived>(ref).*pmf)(std::forward<Args>(args) ...);
}

template <typename Base, typename T, typename RefWrap, typename ... Args>
auto invokeImpl(
    T Base::*pmf,
    RefWrap &&ref,
    Args && ... args
) noexcept(noexcept((ref.get().*pmf)(std::forward<Args>(args) ...)))
-> std::enable_if_t<
    is_function_v<T> &&
    is_reference_wrapper_v<std::decay_t<RefWrap>>,
    decltype((ref.get().*pmf)(std::forward<Args>(args) ...))
>
{
    return (ref.get().*pmf)(std::forward<Args>(args) ...);
}

template <typename Base, typename T, typename Pointer, typename ... Args>
auto invokeImpl(
    T Base::*pmf,
    Pointer &&ptr,
    Args && ... args
) noexcept(noexcept(((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args) ...)))
-> std::enable_if_t<
    is_function_v<T> &&
   !is_reference_wrapper_v<std::decay_t<Pointer>> &&
   !is_base_of_v<Base, std::decay_t<Pointer>>,
    decltype(((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args) ...))
>
{
    return ((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args) ...);
}

template <typename Base, typename T, typename Derived>
auto invokeImpl(T Base::*pmd, Derived &&ref) noexcept(noexcept(std::forward<Derived>(ref).*pmd))
-> std::enable_if_t<
   !is_function_v<T> &&
    is_base_of_v<Base, std::decay_t<Derived>>,
    decltype(std::forward<Derived>(ref).*pmd)
>
{
    return std::forward<Derived>(ref).*pmd;
}

template <typename Base, typename T, typename RefWrap>
auto invokeImpl(T Base::*pmd, RefWrap &&ref) noexcept(noexcept(ref.get().*pmd))
-> std::enable_if_t<
   !is_function_v<T> &&
    is_reference_wrapper_v<std::decay_t<RefWrap>>,
    decltype(ref.get().*pmd)
>
{
    return ref.get().*pmd;
}

template <typename Base, typename T, typename Pointer>
auto invokeImpl(T Base::*pmd, Pointer &&ptr) noexcept(noexcept((*std::forward<Pointer>(ptr)).*pmd))
 -> std::enable_if_t<
   !is_function_v<T> &&
   !is_reference_wrapper_v<std::decay_t<Pointer>> &&
   !is_base_of_v<Base, std::decay_t<Pointer>>,
    decltype((*std::forward<Pointer>(ptr)).*pmd)
>
{
    return (*std::forward<Pointer>(ptr)).*pmd;
}

template <typename F, typename ... Args>
auto invokeImpl(F &&f, Args && ... args) noexcept(noexcept(std::forward<F>(f)(std::forward<Args>(args) ...)))
 -> std::enable_if_t<!is_member_pointer_v<std::decay_t<F>>, decltype(std::forward<F>(f)(std::forward<Args>(args) ...))
>
{
    return std::forward<F>(f)(std::forward<Args>(args) ...);
}
}

template< typename F, typename ... ArgTypes >
auto invoke(F && f, ArgTypes && ... args)
    noexcept(noexcept(Internal::invokeImpl(std::forward<F>(f), std::forward<ArgTypes>(args) ...)))
{
    return Internal::invokeImpl(std::forward<F>(f), std::forward<ArgTypes>(args) ...);
}
}
}

#pragma ide diagnostic pop

#endif /* SIMPLERPC_FUNCTIONAL_H */
