/* Basic type inspectors */

#ifndef SIMPLERPC_INSPECTOR_H
#define SIMPLERPC_INSPECTOR_H

#include <tuple>
#include <memory>
#include <string>
#include <vector>
#include <typeinfo>
#include <type_traits>
#include <unordered_map>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "TypeInfo.h"
#include "Variant.h"
#include "Exceptions.h"
#include "Functional.h"

namespace SimpleRPC
{
/* forward declaration of class `Serializable` must be put here, it is not belonging to `Internal` namespace */
class Serializable;

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
    explicit Field(const std::string &name, const Type &type, size_t offset) : _type(type), _name(name), _offset(offset) {}

public:
    size_t offset(void) const { return _offset; }

public:
    const Type &type(void) const { return _type; }
    const std::string &name(void) const { return _name; }

public:
    template <typename T>
    T &data(void *self) const { return *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(self) + _offset); }

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
    typedef std::function<Variant(::SimpleRPC::Serializable *, const Variant &)> Proxy;

private:
    Proxy _proxy;
    std::vector<Type> _args;

public:
    explicit Method(const char *name, Type &&result, std::vector<Type> &&args, Proxy &&proxy) :
        _name(name), _args(std::move(args)), _result(std::move(result)), _proxy(std::move(proxy)) {}

public:
    const Type &result(void) const { return _result; }
    const std::string &name(void) const { return _name; }
    const std::vector<Type> &args(void) const { return _args; }

public:
    Variant invoke(::SimpleRPC::Serializable *self, const Variant &args) const { return _proxy(self, args); }

};

/****** Reflection registry ******/

struct Registry
{
    class Meta
    {
        Meta(const Meta &) = delete;
        Meta &operator=(const Meta &) = delete;

    public:
        typedef std::unordered_map<std::string, std::shared_ptr<Field>> FieldMap;
        typedef std::unordered_map<std::string, std::shared_ptr<Method>> MethodMap;

    public:
        typedef ::SimpleRPC::Serializable *(* Constructor)(void);

    private:
        FieldMap _fields;
        MethodMap _methods;
        Constructor _constructor;

    private:
        std::string _name;

    public:
        explicit Meta() : _constructor(nullptr) {}
        explicit Meta(const char *name, FieldMap &&fields, MethodMap &&methods, Constructor &&constructor) :
            _name(name), _fields(std::move(fields)), _methods(std::move(methods)), _constructor(std::move(constructor)) {}

    public:
        Meta(Meta &&other)
        {
            std::swap(_fields, other._fields);
            std::swap(_methods, other._methods);
            std::swap(_constructor, other._constructor);
        }

    public:
        const std::string &name(void) const { return _name; }

    public:
        const FieldMap &fields(void) const { return _fields; }
        const MethodMap &methods(void) const { return _methods; }

    public:
        template <typename T> T *newInstance(void) const { return static_cast<T *>(_constructor()); }

    };

private:
    Registry() = delete;
   ~Registry() = delete;

private:
    Registry(const Registry &) = delete;
    Registry &operator=(const Registry &) = delete;

public:
    static void addClass(const std::string &name, std::shared_ptr<Meta> &&meta);
    static const Meta &findClass(const std::string &name);

};

/****** Parameter tuple expanders ******/

template <typename ... Args>
struct ParamTuple;

template <typename Item, typename ... Items>
struct ParamTuple<Item, Items ...>
{
    static std::tuple<Item, Items ...> expand(const Variant &array, int pos = 0)
    {
        return std::tuple_cat(
            std::make_tuple<Item>(static_cast<Item>(array[pos].as<Item>())),
            ParamTuple<Items ...>::expand(array, pos + 1)
        );
    }
};

template <>
struct ParamTuple<>
{
    static std::tuple<> expand(const Variant &, int)
    {
        /* final recursion, no arguments left */
        return std::tuple<>();
    }
};

/****** Reflection registry descriptor ******/

template <typename T>
struct Descriptor
{
    static_assert(std::is_convertible<T *, ::SimpleRPC::Serializable *>::value, "Cannot serialize or deserialize arbitrary type");

public:
    struct MemberData
    {
        bool isMethod;
        std::shared_ptr<Field> field = nullptr;
        std::shared_ptr<Method> method = nullptr;

    public:
        template <typename FieldType>
        explicit MemberData(const std::string &name, const FieldType &field) :
            isMethod(false), field(new Field(name, TypeItem<FieldType>::type(), reinterpret_cast<uintptr_t>(&field))) {}

    public:
        template <typename Result, typename ... Args>
        explicit MemberData(Result (T::*&&method)(Args ...)) :
            isMethod(true), method(new Method(
                typeid(method).name(),
                TypeItem<Result>::type(),
                TypeArray<Args ...>::type(),
                [=](::SimpleRPC::Serializable *self, const Variant &argv)
                {
                    /* simple proxy lambda to invoke target method with parameter array */
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

        Registry::addClass(
            typeid(T).name(),
            std::make_shared<Registry::Meta>(
                typeid(T).name(),
                std::move(fields),
                std::move(methods),
                []{ return static_cast<::SimpleRPC::Serializable *>(new T); }
            )
        );
    }
};
}
}

#endif /* SIMPLERPC_INSPECTOR_H */
