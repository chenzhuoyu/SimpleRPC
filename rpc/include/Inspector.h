/* Basic type inspectors */

#ifndef SIMPLERPC_INSPECTOR_H
#define SIMPLERPC_INSPECTOR_H

#include <tuple>
#include <memory>
#include <string>
#include <vector>
#include <typeinfo>
#include <functional>
#include <type_traits>
#include <unordered_map>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "Variant.h"
#include "TypeInfo.h"
#include "Registry.h"
#include "Exceptions.h"
#include "Functional.h"
#include "TypeWrapper.h"

namespace SimpleRPC
{
/****** Field meta-data ******/

class Field
{
    Type _type;
    size_t _offset;
    std::string _name;

private:
    Field(const Field &) = delete;
    Field &operator=(const Field &) = delete;

public:
    typedef std::function<Variant(const Field *, const void *)> Serializer;
    typedef std::function<void(const Field *, void *, const Variant &)> Deserializer;

private:
    Serializer _serializer;
    Deserializer _deserializer;

public:
    explicit Field(std::string &&name, Type &&type, size_t offset, Serializer &&serializer, Deserializer &&deserializer) :
        _offset         (offset),
        _name           (std::move(name)),
        _type           (std::move(type)),
        _serializer     (std::move(serializer)),
        _deserializer   (std::move(deserializer)) {}

public:
    size_t offset(void) const { return _offset; }

public:
    const Type &type(void) const { return _type; }
    const std::string &name(void) const { return _name; }

public:
    Variant serialize(const void *self) const { return _serializer(this, self); }

public:
    void deserialize(void *self, const Variant &value) { _deserializer(this, self, value); }

public:
    template <typename T>
    T &data(void *self) const { return *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(self) + _offset); }

public:
    template <typename T>
    const T &data(const void *self) const { return *reinterpret_cast<const T *>(reinterpret_cast<uintptr_t>(self) + _offset); }

};

/****** Method meta-data ******/

class Method
{
    Type _result;
    std::string _name;
    std::string _signature;

private:
    Method(const Method &) = delete;
    Method &operator=(const Method &) = delete;

public:
    typedef std::function<Variant(Serializable *, Variant &)> Proxy;

private:
    Proxy _proxy;
    std::vector<Type> _args;

public:
    template <typename R, typename ... Args>
    explicit Method(Internal::MetaMethod<R, Args ...> &&method, Proxy &&proxy) :
        _proxy      (std::move(proxy)),
        _args       (std::move(method.args)),
        _name       (std::move(method.name)),
        _result     (std::move(method.result)),
        _signature  (std::move(method.signature)) {}

public:
    const Type &result(void) const { return _result; }
    const std::vector<Type> &args(void) const { return _args; }

public:
    const std::string &name(void) const { return _name; }
    const std::string &signature(void) const { return _signature; }

public:
    Variant invoke(Serializable *self, Variant &args) const { return _proxy(self, args); }
    Variant invoke(Serializable *self, Variant &&args) const { return _proxy(self, args); }

};

namespace Internal
{
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCSimplifyInspection"

/****** Parameter type reference ******/

template <typename T>
struct TypeRef
{
    typedef std::decay_t<T> U;
    typedef std::conditional_t<IsMap<U>::value || IsVector<U>::value || std::is_convertible<U *, Serializable *>::value, TypeWrapper<U>, T> Type;
};

/****** Parameter tuple expanders ******/

template <size_t I, typename ... Args>
struct ParamTupleImpl;

template <size_t I, typename Item, typename ... Items>
struct ParamTupleImpl<I, Item, Items ...>
{
    static constexpr std::tuple<Item, Items ...> expand(Variant &array)
    {
        return std::tuple_cat(
            std::forward_as_tuple(array[I].get<Item>()),
            ParamTupleImpl<I + 1, Items ...>::expand(array)
        );
    }
};

template <size_t I>
struct ParamTupleImpl<I>
{
    static constexpr std::tuple<> expand(Variant &)
    {
        /* final recursion, no arguments left */
        return std::tuple<>();
    }
};

template <typename ... Args>
using ParamTuple = ParamTupleImpl<0, typename TypeRef<Args>::Type ...>;

/****** Mutable parameter back-patcher ******/

template <size_t I, typename T, typename U, bool IsObject>
struct ObjectPatcher
{
    static void patch(Variant &array, U &item)
    {
        /* read-only argument, no need to patch */
        /* mutable reference of primitive types are overwritten directly back into `Variant` */
        /* thus do nothing */
    }
};

template <size_t I, typename T, typename U>
struct ObjectPatcher<I, T, U, true>
{
    static void patch(Variant &array, U &item)
    {
        /* for objects, re-serialize to `Variant` */
        array[I] = (*item).serialize();
    }
};

template <size_t I, typename T>
struct ItemPatcher
{
    template <typename U>
    static void patch(Variant &array, U &item)
    {
        /* delegate to object patcher to get around the template problem */
        ObjectPatcher<I, T, U, IsMutableObjectReference<T>::value>::patch(array, item);
    }
};

template <size_t I, typename T>
struct ItemPatcher<I, std::vector<T> &>
{
    template <typename U>
    static void patch(Variant &array, U &item)
    {
        /* for arrays, simply assign back */
        array[I] = std::move(*item);
    }
};

template <size_t I, typename K, typename V>
struct ItemPatcher<I, std::map<K, V> &>
{
    template <typename U>
    static void patch(Variant &array, U &item)
    {
        /* for tree maps, simply assign back too */
        array[I] = std::move(*item);
    }
};

template <size_t I, typename K, typename V>
struct ItemPatcher<I, std::unordered_map<K, V> &>
{
    template <typename U>
    static void patch(Variant &array, U &item)
    {
        /* for hash maps, simply assign back too */
        array[I] = std::move(*item);
    }
};

template <size_t I, typename Tuple, typename ... Args>
struct BackPatcherImpl;

template <size_t I, typename Tuple, typename Item, typename ... Items>
struct BackPatcherImpl<I, Tuple, Item, Items ...>
{
    static void patch(Variant &array, Tuple &&tuple)
    {
        ItemPatcher<I, Item>::template patch(array, std::get<I>(tuple));
        BackPatcherImpl<I + 1, Tuple, Items ...>::patch(array, std::move(tuple));
    }
};

template <size_t I, typename Tuple>
struct BackPatcherImpl<I, Tuple>
{
    static void patch(Variant &, Tuple &&)
    {
        /* final recursion, no arguments left */
        /* thus nothing to do */
    }
};

template <typename Tuple, typename ... Args>
using BackPatcher = BackPatcherImpl<0, Tuple, Args ...>;

#pragma clang diagnostic pop
}

/****** Reflection registry descriptor ******/

template <typename T>
struct Descriptor
{
    static_assert(std::is_convertible<T *, Serializable *>::value, "Cannot serialize or deserialize arbitrary type");

public:
    struct MemberData
    {
        bool isMethod;
        std::shared_ptr<Field> field = nullptr;
        std::shared_ptr<Method> method = nullptr;

    public:
        template <typename FieldType>
        explicit MemberData(const char *name, FieldType &ref) :
            isMethod(false), field(new Field(
                std::string(name),
                Internal::TypeItem<FieldType>::type(),
                reinterpret_cast<uintptr_t>(&ref),
                [](const Field *field, const void *self)
                {
                    /* using the template to resolve types during compilation */
                    return Variant(field->data<FieldType>(self));
                },
                [](const Field *field, void *self, const Variant &value)
                {
                    /* write the value directly back into field */
                    field->data<FieldType>(self) = value.get<FieldType>();
                }
            ))
        {
            /* reference field types are not supported */
            static_assert(!std::is_reference<FieldType>::value, "Reference field types are not supported");
        }

    public:
        template <typename Result, typename ... Args>
        explicit MemberData(const char *name, Result (T::*&&method)(Args ...)) :
            isMethod(true), method(new Method(
                Internal::MetaMethod<Result, Args ...>(name),
                [f = std::move(method)](Serializable *self, Variant &argv) mutable
                {
                    /* wrapped argument types tuple */
                    typedef std::tuple<typename Internal::TypeRef<Args>::Type ...> Tuple;
                    typedef Internal::Functional::MetaFunction<Variant, Result, T, Tuple, Args ...> MetaFunction;

                    /* check for parameters */
                    if (argv.type() != Type::TypeCode::Array)
                        throw Exceptions::TypeError("Parameter pack must be an array");
                    else if (argv.size() != sizeof ... (Args))
                        throw Exceptions::ArgumentError(sizeof ... (Args), argv.size());

                    /* build arguments tuple and invoke target method through meta function wrapper */
                    auto tuple = Internal::ParamTuple<Args ...>::expand(argv);
                    auto result = MetaFunction::invoke(static_cast<T *>(self), std::move(f), tuple);

                    /* patch mutable arguments back into `argv` */
                    Internal::BackPatcher<Tuple, Args ...>::patch(argv, std::move(tuple));
                    return std::move(result);
                }
            ))
        {
            /* reference return types are not supported */
            static_assert(!std::is_reference<Result>::value, "Reference return types are not supported");
        }
    };

public:
    explicit Descriptor(const std::vector<MemberData> &members)
    {
        Registry::Meta::FieldMap fields;
        Registry::Meta::MethodMap methods;

        for (const MemberData &info : members)
        {
            if (!info.isMethod)
                fields.emplace(info.field->name(), std::move(info.field));
            else
                methods.emplace(info.method->signature(), std::move(info.method));
        }

        Registry::addClass(std::make_shared<Registry::Meta>(
            Internal::TypeItem<T>::type().toSignature(),
            std::move(fields),
            std::move(methods),
            []{ return static_cast<Serializable *>(new T); }
        ));
    }
};
}

#endif /* SIMPLERPC_INSPECTOR_H */
