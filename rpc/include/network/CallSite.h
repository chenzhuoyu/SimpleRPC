/* Abstraction call site */

#ifndef SIMPLERPC_CALLSITE_H
#define SIMPLERPC_CALLSITE_H

#include <string>
#include "Variant.h"
#include "TypeInfo.h"
#include "Exceptions.h"
#include "TypeWrapper.h"

namespace SimpleRPC
{
namespace Network
{
namespace Helpers
{
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCSimplifyInspection"

template <typename T, bool IsComposite>
struct CompositeUnwrapper;

template <typename T>
struct CompositeUnwrapper<T, true>
{
    static T unwrap(Internal::Variant &&value)
    {
        /* composite value, unwrap from type wrapper */
        return std::move(*value.get<Internal::TypeWrapper<T>>());
    }
};

template <typename T>
struct CompositeUnwrapper<T, false>
{
    static T unwrap(Internal::Variant &&value)
    {
        /* simple value, use standard getter */
        return std::move(value.get<T>());
    }
};

template <typename T>
struct Unwrapper
{
    typedef std::decay_t<T> U;
    static U unwrap(Internal::Variant &&value)
    {
        return CompositeUnwrapper<
            U,
            Internal::IsVector<U>::value ||
            std::is_convertible<U *, Internal::Serializable *>::value
        >::unwrap(std::move(value));
    }
};

template <>
struct Unwrapper<void>
{
    static void unwrap(Internal::Variant &&value)
    {
        /* void return type, check for void variant only */
        if (value.type() != Internal::Type::TypeCode::Void)
            throw Exceptions::TypeError(value.toString() + " is not void");
    }
};

template <size_t I, typename T, typename U, bool IsObject, bool IsMutable>
struct ObjectPatcher
{
    static void patch(Internal::Variant &array, U &&item)
    {
        /* read-only argument, no need to patch */
        /* thus do nothing */
    }
};

template <size_t I, typename T, typename U>
struct ObjectPatcher<I, T, U, true, true>
{
    static void patch(Internal::Variant &array, U &&item)
    {
        /* for objects, deserialize from `Variant` */
        item.deserialize(std::move(array[I]));
    }
};

template <size_t I, typename T, typename U>
struct ObjectPatcher<I, T, U, false, true>
{
    static void patch(Internal::Variant &array, U &&item)
    {
        /* for non-objects, simply assign back */
        item = std::move(Unwrapper<U>::unwrap(std::move(array[I])));
    }
};

template <size_t I, typename T>
struct ItemPatcher
{
    template <typename U>
    static void patch(Internal::Variant &array, U &&item)
    {
        /* delegate to object patcher to get around the template problem */
        ObjectPatcher<
            I,
            T,
            U,
            Internal::IsObjectReference<T>::value,
            Internal::IsMutableReference<T>::value
        >::patch(array, std::forward<U>(item));
    }
};

template <size_t I, typename ... Args>
struct BackPatcherImpl;

template <size_t I, typename Arg, typename ... Args>
struct BackPatcherImpl<I, Arg, Args ...>
{
    static void patch(Internal::Variant &&value, Arg &&arg, Args && ... args)
    {
        ItemPatcher<I, Arg>::patch(value, std::forward<Arg>(arg));
        BackPatcherImpl<I + 1, Args ...>::patch(std::move(value), std::forward<Args>(args) ...);
    }
};

template <size_t I>
struct BackPatcherImpl<I>
{
    static void patch(Internal::Variant &&value)
    {
        /* the last recursion, no arguments left */
        /* thus nothing to do */
    }
};

template <typename ... Args>
using BackPatcher = BackPatcherImpl<0, Args ...>;

#pragma clang diagnostic pop
}

class CallSite
{
    CallSite(const CallSite &) = delete;
    CallSite &operator=(const CallSite &) = delete;

public:
    virtual ~CallSite() {}
    explicit CallSite() {}

public:
    virtual void cleanup(size_t id) noexcept = 0;
    virtual size_t startup(const std::string &name) = 0;

public:
    virtual Internal::Variant invoke(size_t id, std::string &&method, Internal::Variant &args) = 0;
    virtual Internal::Variant invoke(size_t id, std::string &&method, Internal::Variant &&args) { return invoke(id, std::move(method), args); }

public:
    template <typename R, typename ... Args>
    R invoke(size_t id, Internal::MetaMethod<R, Args ...> &&method, Args && ... args)
    {
        /* construct argument pack, and delegate to real call site */
        Internal::Variant argv = Internal::Variant::array(std::forward<Args>(args) ...);
        Internal::Variant result = invoke(id, std::move(method.signature), argv);

        /* patch values back from variants, this is required to support mutable reference */
        Helpers::BackPatcher<Args ...>::patch(std::move(argv), std::forward<Args>(args) ...);

        /* unwrap result from variant */
        return Helpers::Unwrapper<R>::unwrap(std::move(result));
    };
};
}
}

#endif /* SIMPLERPC_CALLSITE_H */
