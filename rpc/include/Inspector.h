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

namespace SimpleRPC
{
namespace Internal
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
    explicit Field(const std::string &name, const Type &type, size_t offset, Serializer &&serializer, Deserializer &&deserializer) :
        _type(type), _name(name), _offset(offset), _serializer(serializer), _deserializer(deserializer) {}

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

private:
    Method(const Method &) = delete;
    Method &operator=(const Method &) = delete;

public:
    typedef std::function<Variant(Serializable *, const Variant &)> Proxy;

private:
    Proxy _proxy;
    std::vector<Type> _args;

public:
    explicit Method(const char *name, Type &&result, std::vector<Type> &&args, std::string &&signature, Proxy &&proxy) :
        _args(std::move(args)), _name(name + signature), _proxy(std::move(proxy)), _result(std::move(result)) {}

public:
    const Type &result(void) const { return _result; }
    const std::string &name(void) const { return _name; }
    const std::vector<Type> &args(void) const { return _args; }

public:
    Variant invoke(Serializable *self, const Variant &args) const { return _proxy(self, args); }

};

/****** Parameter tuple expanders ******/

template <typename ... Args>
struct ParamTuple;

template <typename Item, typename ... Items>
struct ParamTuple<Item, Items ...>
{
    static std::tuple<Item, Items ...> expand(const Variant &array)
    {
        const size_t p = array.size() - sizeof ... (Items) - 1;
        return std::tuple_cat(std::make_tuple(array[p].get<Item>()), ParamTuple<Items ...>::expand(array));
    }
};

template <>
struct ParamTuple<>
{
    static std::tuple<> expand(const Variant &)
    {
        /* final recursion, no arguments left */
        return std::tuple<>();
    }
};

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
        explicit MemberData(const std::string &name, const FieldType &reference) :
            isMethod(false), field(new Field(
                name,
                TypeItem<FieldType>::type(),
                reinterpret_cast<uintptr_t>(&reference),
                [=](const Field *field, const void *self)
                {
                    /* using the template to resolve types during compilation */
                    return Variant(field->data<FieldType>(self));
                },
                [=](const Field *field, void *self, const Variant &value)
                {
                    /* using the template to resolve types during compilation */
                    field->data<FieldType>(self) = value.get<FieldType>();
                }
            )) {}

    public:
        template <typename Result, typename ... Args>
        explicit MemberData(const char *name, Result (T::*&&method)(Args ...)) :
            isMethod(true), method(new Method(
                name,
                TypeItem<Result>::type(),
                TypeArray<Args ...>::type(),
                Signature<Result (T::*)(Args ...)>::resolve(),
                [=](Serializable *self, const Variant &argv)
                {
                    /* check for parameter count and invoke target method with parameter array */
                    if (argv.size() != sizeof ... (Args))
                        throw Exceptions::ArgumentError(sizeof ... (Args), argv.size());
                    else
                        return Variant(Functional::apply(static_cast<T *>(self), method, ParamTuple<Args ...>::expand(argv)));
                }
            )) {}
    };

public:
    explicit Descriptor(const std::vector<MemberData> &members)
    {
        Registry::Meta::FieldMap fields;
        Registry::Meta::MethodMap methods;

        for (const MemberData &info : members)
        {
            if (!info.isMethod)
                fields.insert({ info.field->name(), std::move(info.field) });
            else
                methods.insert({ info.method->name(), std::move(info.method) });
        }

        Registry::addClass(std::make_shared<Registry::Meta>(
            Signature<T>::resolve(),
            std::move(fields),
            std::move(methods),
            []{ return static_cast<Serializable *>(new T); }
        ));
    }
};
}
}

#endif /* SIMPLERPC_INSPECTOR_H */
