/* Type information descriptor */

#ifndef SIMPLERPC_TYPEINFO_H
#define SIMPLERPC_TYPEINFO_H

#include <string>
#include <memory>

#include "Exceptions.h"
#include "Functional.h"

namespace SimpleRPC
{
namespace Internal
{
/****** Type descriptor ******/

class Type final
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
    Type(TypeCode typeCode) : _typeCode(typeCode)
    {
        if (typeCode == TypeCode::Array || typeCode == TypeCode::Struct)
        {
            fprintf(stderr, "assert_failed(): typeCode != TypeCode::Array && typeCode != TypeCode::Struct");
            abort();
        }
    }

public:
    /* array sub-item type */
    explicit Type(TypeCode typeCode, const Type &itemType) : _typeCode(TypeCode::Array), _itemType(new Type(itemType))
    {
        if (typeCode != TypeCode::Array)
        {
            fprintf(stderr, "assert_failed(): typeCode == TypeCode::Array");
            abort();
        }
    }

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

/****** Type resolvers ******/

/* forward declaration of class Serializable */
class Serializable;

/* type size container */
template <size_t size>
struct TypeSize
{
    static Type signedType(void)
    {
        /* these `if` statement would be optimized away by compiler */
        if (size == sizeof(int8_t )) return Type::TypeCode::Int8;
        if (size == sizeof(int16_t)) return Type::TypeCode::Int16;
        if (size == sizeof(int32_t)) return Type::TypeCode::Int32;
        if (size == sizeof(int64_t)) return Type::TypeCode::Int64;

        /* should never reaches here */
        static_assert(size == sizeof(int8_t) || size == sizeof(int16_t) || size == sizeof(int32_t) || size == sizeof(int64_t), "Unknown signed integer size");
        abort();
    }

    static Type unsignedType(void)
    {
        /* these `if` statement would be optimized away by compiler */
        if (size == sizeof(uint8_t )) return Type::TypeCode::UInt8;
        if (size == sizeof(uint16_t)) return Type::TypeCode::UInt16;
        if (size == sizeof(uint32_t)) return Type::TypeCode::UInt32;
        if (size == sizeof(uint64_t)) return Type::TypeCode::UInt64;

        /* should never reaches here */
        static_assert(size == sizeof(uint8_t) || size == sizeof(uint16_t) || size == sizeof(uint32_t) || size == sizeof(uint64_t), "Unknown unsigned integer size");
        abort();
    }
};

template <bool isSigned, bool isUnsigned, bool isStructLike, typename Item>
struct TypeHelper
{
    static Type type(void)
    {
        /* these `if` statement would be optimized away by compiler */
        if (isSigned)       return TypeSize<sizeof(Item)>::signedType();
        if (isUnsigned)     return TypeSize<sizeof(Item)>::unsignedType();
        if (isStructLike)   return Type(Type::TypeCode::Struct, typeid(Item).name());

        /* should never reaches here */
        static_assert(isSigned || isUnsigned || isStructLike, "Cannot serialize or deserialize arbitrary type");
        abort();
    }
};

template <typename Item>
struct TypeItem
{
    static Type type(void)
    {
        /* invoke type helper for detailed type information */
        return TypeHelper<
            std::is_signed<Item>::value,
            std::is_unsigned<Item>::value,
            std::is_convertible<Item *, Serializable *>::value,
            Item
        >::type();
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

/* floating point numbers */
template <> struct TypeItem<float > { static Type type(void) { return Type::TypeCode::Float; } };
template <> struct TypeItem<double> { static Type type(void) { return Type::TypeCode::Double; } };

/* STL string */
template <> struct TypeItem<std::string> { static Type type(void) { return Type::TypeCode::String; } };

/****** Type array resolvers ******/

template <typename ... Items>
struct TypeArray;

template <typename Item, typename ... Items>
struct TypeArray<Item, Items ...>
{
    static std::vector<Type> type(void)
    {
        auto first = std::move(TypeItem<Item>::type());
        auto remains = std::move(TypeArray<Items ...>::type());

        remains.insert(remains.begin(), first);
        return remains;
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
}
}

#endif /* SIMPLERPC_TYPEINFO_H */
