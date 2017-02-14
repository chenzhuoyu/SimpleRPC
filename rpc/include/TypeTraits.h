/* Basic type-traits */

#ifndef SIMPLERPC_TYPETRAITS_H
#define SIMPLERPC_TYPETRAITS_H

#include <mutex>
#include <memory>
#include <string>
#include <vector>
#include <typeinfo>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include <stdio.h>
#include <cxxabi.h>
#include <stdint.h>
#include <stdlib.h>

#include "Exceptions.h"

namespace SimpleRPC
{
/****** Type information ******/

class Type
{
    std::string _className;
    std::shared_ptr<Type> _itemType;

public:
    enum class TypeCode : int
    {
        /* signed integers */
        Int8,
        Int16,
        Int32,
        Int64,

        /* unsigned integers */
        UInt8,
        UInt16,
        UInt32,
        UInt64,

        /* floating point numbers */
        Float,
        Double,

        /* STL string */
        String,

        /* compond types */
        Array,
        Struct,

        /* used for counting number of types, not actually a type */
        TypeCount
    };

private:
    TypeCode _typeCode;

public:
    /* primitive types */
    explicit Type(TypeCode typeCode) : _typeCode(typeCode) {}

public:
    /* struct type */
    explicit Type(TypeCode typeCode, const std::string &className) : _typeCode(TypeCode::Struct), _className(className)
    {
        if (typeCode != TypeCode::Struct)
        {
            fprintf(stderr, "assert_failed(): typeCode == TypeCode::Struct");
            abort();
        }
    }

public:
    /* array sub-item type */
    explicit Type(TypeCode typeCode, const Type &itemType) : _typeCode(TypeCode::Array), _itemType(new Type(itemType._typeCode))
    {
        if (typeCode != TypeCode::Array)
        {
            fprintf(stderr, "assert_failed(): typeCode == TypeCode::Array");
            abort();
        }

        _itemType->_itemType = itemType._itemType;
        _itemType->_className = itemType._className;
    }

public:
    TypeCode typeCode(void) const { return _typeCode; }
    std::string className(void) const { return _className; }

public:
    const Type &itemType(void) const
    {
        if (_typeCode == TypeCode::Array)
            return *_itemType;
        else
            throw Exceptions::TypeError("Only arrays have sub-items");
    }

public:
    std::string toString(void) const
    {
        switch (_typeCode)
        {
            case TypeCode::Int8     : return "int8_t";
            case TypeCode::Int16    : return "int16_t";
            case TypeCode::Int32    : return "int32_t";
            case TypeCode::Int64    : return "int64_t";

            case TypeCode::UInt8    : return "uint8_t";
            case TypeCode::UInt16   : return "uint16_t";
            case TypeCode::UInt32   : return "uint32_t";
            case TypeCode::UInt64   : return "uint64_t";

            case TypeCode::Float    : return "float";
            case TypeCode::Double   : return "double";

            case TypeCode::Struct   : return _className;
            case TypeCode::String   : return "std::string";
            case TypeCode::Array    : return "std::vector<" + _itemType->toString() + ">";

            default:
            {
                fprintf(stderr, "*** FATAL: impossible type %d\n", static_cast<int>(_typeCode));
                abort();
            }
        }
    }
};

/****** Field meta-data ******/

class Field
{
    Type _type;
    bool _isRequired;

private:
    size_t _offset;
    std::string _name;

public:
    explicit Field(const std::string &name, const Type &type, size_t offset, bool isRequired) :
        _type(type), _name(name), _offset(offset), _isRequired(isRequired) {}

public:
    size_t offset(void) const { return _offset; }
    std::string name(void) const { return _name; }

public:
    const Type &type(void) const { return _type; }

public:
    bool isRequired(void) const { return _isRequired; }

public:
    template <typename T>
    T &data(void *self) const { return *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(self) + _offset); }

};

/****** Reflection registry ******/

class Serializable;
struct Registry
{
    struct Meta
    {
        typedef Serializable *(* Constructor)(void);
        typedef std::unordered_map<std::string, Field> FieldData;

    public:
        FieldData fields;
        Constructor constructor;

    public:
        explicit Meta(const Constructor &constructor) : constructor(constructor) {}

    };

public:
    static void addClass(const std::string &name, const Meta &meta);
    static Meta &findClass(const std::string &name);

};

/****** Type resolvers ******/

/* structs */
template <typename Item>
struct TypeItem
{
    static Type type(void)
    {
        static_assert(std::is_convertible<Item *, Serializable *>::value, "Cannot serialize or deserialize arbitrary type");
        return Type(Type::TypeCode::Struct, typeid(Item).name());
    }
};

/* arrays */
template <typename Item>
struct TypeItem<std::vector<Item>>
{
    static Type type(void)
    {
        /* recursive to arrau item type */
        return Type(Type::TypeCode::Array, TypeItem<Item>::type());
    }
};

/* signed integers */
template <> struct TypeItem<int8_t > { static Type type(void) { return Type(Type::TypeCode::Int8 ); } };
template <> struct TypeItem<int16_t> { static Type type(void) { return Type(Type::TypeCode::Int16); } };
template <> struct TypeItem<int32_t> { static Type type(void) { return Type(Type::TypeCode::Int32); } };
template <> struct TypeItem<int64_t> { static Type type(void) { return Type(Type::TypeCode::Int64); } };

/* unsigned integers */
template <> struct TypeItem<uint8_t > { static Type type(void) { return Type(Type::TypeCode::UInt8 ); } };
template <> struct TypeItem<uint16_t> { static Type type(void) { return Type(Type::TypeCode::UInt16); } };
template <> struct TypeItem<uint32_t> { static Type type(void) { return Type(Type::TypeCode::UInt32); } };
template <> struct TypeItem<uint64_t> { static Type type(void) { return Type(Type::TypeCode::UInt64); } };

/* floating point numbers */
template <> struct TypeItem<float > { static Type type(void) { return Type(Type::TypeCode::Float ); } };
template <> struct TypeItem<double> { static Type type(void) { return Type(Type::TypeCode::Double); } };

/* STL string */
template <> struct TypeItem<std::string> { static Type type(void) { return Type(Type::TypeCode::String); } };

/****** Type array resolvers ******/

template <typename ... Items>
struct TypeArray;

template <typename Item, typename ... Items>
struct TypeArray<Item, Items ...>
{
    static std::vector<Type> type(void)
    {
        std::vector<Type> result;
        std::vector<Type> remains = std::move(TypeArray<Items ...>::type());

        result.push_back(TypeItem<Item>::type());
        result.insert(result.end(), remains.begin(), remains.end());
        return result;
    }
};

template <>
struct TypeArray<>
{
    static std::vector<Type> type(void)
    {
        /* final recursion, no arguments left */
        return std::vector<Type>();
    }
};

/****** Method meta-data ******/

template <typename T>
class Method
{
    Type _result;
    std::vector<Type> _args;

public:
    explicit Method(const Type &result, std::vector<Type> &&args) : _result(result), _args(std::move(args)) {}

};

/****** Reflection registry descriptor ******/

template <typename T>
struct Descriptor
{
    struct MemberData
    {
        bool isMethod;
        std::shared_ptr<Field> field = nullptr;
        std::shared_ptr<Method<T>> method = nullptr;

    public:
        template <typename FieldType>
        explicit MemberData(const std::string &name, const FieldType &field, bool isRequired) :
            isMethod(false), field(new Field(name, TypeItem<FieldType>::type(), reinterpret_cast<uintptr_t>(&field), isRequired)) {}

    public:
        template <typename Result, typename ... Args>
        explicit MemberData(Result (T::*method)(Args ...)) : isMethod(true)
        {
            Type result = TypeItem<Result>::type();
            std::vector<Type> args = TypeArray<Args ...>::type();

            fprintf(stderr, "-----\n");
            for (const auto &type : args)
                fprintf(stderr, "%s\n", type.toString().c_str());
            fprintf(stderr, "-----\n");

            /* WTF is this kind of grammar ??? */
//            T *instance = nullptr /* or arbitrary `T`-typed instance */;
//            Result &&res = (instance->*method)(5, "asd");
        }
    };

public:
    explicit Descriptor(const std::vector<MemberData> &fields)
    {
        /* class meta data */
        Registry::Meta meta([](void) -> Serializable *
        {
            /* just to instaniate corresponding class */
            return new T;
        });

        /* fill fields data */
        for (const MemberData &info : fields)
        {
            if (!info.isMethod)
                meta.fields.insert({ info.field->name(), *info.field });
        }

        /* register to class registry */
        Registry::addClass(typeid(T).name(), meta);
    }
};
}

#endif /* SIMPLERPC_TYPETRAITS_H */
