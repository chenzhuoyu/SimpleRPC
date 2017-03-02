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

        /* boolean */
        Boolean,

        /* STL string */
        String,

        /* compond types */
        Array,
        Object
    };

private:
    TypeCode _typeCode;

public:
    /* primitive types */
    Type(TypeCode typeCode) : _typeCode(typeCode)
    {
        if (typeCode == TypeCode::Array || typeCode == TypeCode::Object)
        {
            fprintf(stderr, "assert_failed(): typeCode != TypeCode::Array && typeCode != TypeCode::Object");
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
    /* object type */
    explicit Type(TypeCode typeCode, const std::string &className) : _typeCode(TypeCode::Object), _className(className)
    {
        if (typeCode != TypeCode::Object)
        {
            fprintf(stderr, "assert_failed(): typeCode == TypeCode::Object");
            abort();
        }
    }

public:
    TypeCode typeCode(void) const { return _typeCode; }
    const std::string &className(void) const { return _className; }

public:
    const Type &itemType(void) const
    {
        if (_typeCode == TypeCode::Array)
            return *_itemType;
        else
            throw Exceptions::TypeError("Only arrays have sub-items");
    }

public:
    std::string toSignature(void) const
    {
        switch (_typeCode)
        {
            case TypeCode::Int8     : return "b";
            case TypeCode::Int16    : return "h";
            case TypeCode::Int32    : return "i";
            case TypeCode::Int64    : return "q";

            case TypeCode::UInt8    : return "B";
            case TypeCode::UInt16   : return "H";
            case TypeCode::UInt32   : return "I";
            case TypeCode::UInt64   : return "Q";

            case TypeCode::Float    : return "f";
            case TypeCode::Double   : return "d";
            case TypeCode::Boolean  : return "?";

            case TypeCode::String   : return "s";
            case TypeCode::Object   : return "{" + _className + "}";
            case TypeCode::Array    : return "[" + _itemType->toSignature() + "]";
        }
    }
};

/****** Type resolvers ******/

/* forward declaration of class Serializable */
class Serializable;

/* type size container */
template <size_t size>
struct TypeSize;

template <>
struct TypeSize<sizeof(int8_t)>
{
    static Type   signedType(void) { return Type::TypeCode:: Int8; }
    static Type unsignedType(void) { return Type::TypeCode::UInt8; }
};

template <>
struct TypeSize<sizeof(int16_t)>
{
    static Type   signedType(void) { return Type::TypeCode:: Int16; }
    static Type unsignedType(void) { return Type::TypeCode::UInt16; }
};

template <>
struct TypeSize<sizeof(int32_t)>
{
    static Type   signedType(void) { return Type::TypeCode:: Int32; }
    static Type unsignedType(void) { return Type::TypeCode::UInt32; }
};

template <>
struct TypeSize<sizeof(int64_t)>
{
    static Type   signedType(void) { return Type::TypeCode:: Int64; }
    static Type unsignedType(void) { return Type::TypeCode::UInt64; }
};

template <bool isSigned, bool isUnsigned, bool isStructLike, typename Item>
struct TypeHelper
{
    /* types that not recognized */
    static_assert(isSigned || isUnsigned || isStructLike, "Cannot serialize or deserialize arbitrary type");
};

template <typename Item>
struct TypeHelper<true, false, false, Item>
{
    static Type type(void)
    {
        /* signed integers */
        return TypeSize<sizeof(Item)>::signedType();
    }
};

template <typename Item>
struct TypeHelper<false, true, false, Item>
{
    static Type type(void)
    {
        /* unsigned integers */
        return TypeSize<sizeof(Item)>::unsignedType();
    }
};

template <typename Item>
struct TypeHelper<false, false, true, Item>
{
    static Type type(void)
    {
        /* structure types */
        return Type(Type::TypeCode::Object, typeid(Item).name());
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

/* boolean */
template <> struct TypeItem<bool> { static Type type(void) { return Type::TypeCode::Boolean; } };

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

/****** Signature generators ******/

template <typename T>
struct Signature
{
    static std::string resolve(void)
    {
        /* simple type, just resolve by `Type` */
        return TypeItem<T>::type().toSignature();
    }
};

template <typename T>
struct Signature<std::vector<T>>
{
    static std::string resolve(void)
    {
        /* arrays, resolve recursively  */
        return "[" + Signature<T>::resolve() + "]";
    }
};

template <typename R, typename T, typename ... Args>
struct Signature<R (T::*)(Args ...)>
{
    static std::string resolve(void)
    {
        /* method signature should be treated seperately */
        std::string result = "(";
        std::vector<Type> &&types = TypeArray<Args ...>::type();

        std::for_each(types.begin(), types.end(), [&](auto x){ result += x.toSignature(); });
        return result + ")" + TypeItem<R>::type().toSignature();
    }
};

/****** Utilities ******/

template <typename T>
struct IsVector : public std::false_type {};

template <typename T>
struct IsVector<std::vector<T>> : public std::true_type
{
    typedef T ItemType;
};
}
}

#endif /* SIMPLERPC_TYPEINFO_H */
