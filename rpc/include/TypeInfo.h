/* Type information descriptor */

#ifndef SIMPLERPC_TYPEINFO_H
#define SIMPLERPC_TYPEINFO_H

#include <string>
#include <memory>
#include <algorithm>

#include "Registry.h"
#include "Exceptions.h"
#include "Functional.h"

namespace SimpleRPC
{
namespace Internal
{
/****** Type descriptor ******/

class Type final
{
    bool _isMutable;
    std::string _className;
    std::shared_ptr<Type> _itemType;

public:
    enum class TypeCode : int
    {
        /* void */
        Void,

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
    /* default void type */
    explicit Type() : _typeCode(TypeCode::Void), _isMutable(false) {}

public:
    /* primitive types */
    explicit Type(TypeCode typeCode, bool isMutable) : _typeCode(typeCode), _isMutable(isMutable)
    {
        if (typeCode == TypeCode::Array || typeCode == TypeCode::Object)
        {
            fprintf(stderr, "assert_failed(): typeCode != TypeCode::Array && typeCode != TypeCode::Object");
            abort();
        }
    }

public:
    /* array sub-item type */
    explicit Type(TypeCode typeCode, Type &&itemType, bool isMutable) :
        _typeCode(TypeCode::Array), _itemType(new Type(std::move(itemType))), _isMutable(isMutable)
    {
        if (itemType.isMutable())
        {
            fprintf(stderr, "assert_failed(): !itemType.isMutable()");
            abort();
        }

        if (typeCode != TypeCode::Array)
        {
            fprintf(stderr, "assert_failed(): typeCode == TypeCode::Array");
            abort();
        }
    }

public:
    /* object type */
    explicit Type(TypeCode typeCode, std::string &&className, bool isMutable) :
        _typeCode(TypeCode::Object), _className(std::move(className)), _isMutable(isMutable)
    {
        if (typeCode != TypeCode::Object)
        {
            fprintf(stderr, "assert_failed(): typeCode == TypeCode::Object");
            abort();
        }
    }

public:
    bool isMutable(void) const { return _isMutable; }
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
        if (!_isMutable)
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

                case TypeCode::Void     : return "v";
                case TypeCode::String   : return "s";
                case TypeCode::Object   : return "{" + _className + "}";
                case TypeCode::Array    : return "[" + _itemType->toSignature() + "]";
            }
        }
        else
        {
            switch (_typeCode)
            {
                case TypeCode::Int8     : return "b&";
                case TypeCode::Int16    : return "h&";
                case TypeCode::Int32    : return "i&";
                case TypeCode::Int64    : return "q&";

                case TypeCode::UInt8    : return "B&";
                case TypeCode::UInt16   : return "H&";
                case TypeCode::UInt32   : return "I&";
                case TypeCode::UInt64   : return "Q&";

                case TypeCode::Float    : return "f&";
                case TypeCode::Double   : return "d&";
                case TypeCode::Boolean  : return "?&";

                case TypeCode::String   : return "s&";
                case TypeCode::Object   : return "{" + _className + "}&";
                case TypeCode::Array    : return "[" + _itemType->toSignature() + "]&";

                case TypeCode::Void:
                    throw std::range_error("`void` should always be immutable");
            }
        }
    }
};

/****** Type resolvers ******/

/* type size container */
template <size_t size>
struct TypeSize;

template <>
struct TypeSize<sizeof(int8_t)>
{
    static Type   signedType(bool isMutable) { return Type(Type::TypeCode:: Int8, isMutable); }
    static Type unsignedType(bool isMutable) { return Type(Type::TypeCode::UInt8, isMutable); }
};

template <>
struct TypeSize<sizeof(int16_t)>
{
    static Type   signedType(bool isMutable) { return Type(Type::TypeCode:: Int16, isMutable); }
    static Type unsignedType(bool isMutable) { return Type(Type::TypeCode::UInt16, isMutable); }
};

template <>
struct TypeSize<sizeof(int32_t)>
{
    static Type   signedType(bool isMutable) { return Type(Type::TypeCode:: Int32, isMutable); }
    static Type unsignedType(bool isMutable) { return Type(Type::TypeCode::UInt32, isMutable); }
};

template <>
struct TypeSize<sizeof(int64_t)>
{
    static Type   signedType(bool isMutable) { return Type(Type::TypeCode:: Int64, isMutable); }
    static Type unsignedType(bool isMutable) { return Type(Type::TypeCode::UInt64, isMutable); }
};

template <bool isSigned, bool isUnsigned, bool isStructLike, typename Item>
struct TypeHelper
{
    static Type type(bool isMutable)
    {
        /* types that not recognized */
        static_assert(isSigned || isUnsigned || isStructLike, "Cannot serialize or deserialize arbitrary type");
        abort();
    }
};

template <typename Item>
struct TypeHelper<true, false, false, Item>
{
    static Type type(bool isMutable)
    {
        /* signed integers */
        return TypeSize<sizeof(Item)>::signedType(isMutable);
    }
};

template <typename Item>
struct TypeHelper<false, true, false, Item>
{
    static Type type(bool isMutable)
    {
        /* unsigned integers */
        return TypeSize<sizeof(Item)>::unsignedType(isMutable);
    }
};

template <typename Item>
struct TypeHelper<false, false, true, Item>
{
    static Type type(bool isMutable)
    {
        /* structure types */
        return Type(Type::TypeCode::Object, typeid(Item).name(), isMutable);
    }
};

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCSimplifyInspection"

template <typename Item>
struct TypeItem
{
    static Type type(void)
    {
        /* doesn't support pointers or R-Value references */
        static_assert(!std::is_pointer<Item>::value, "Pointers are not supported");
        static_assert(!std::is_rvalue_reference<Item>::value, "R-Value references are not supported");

        /* invoke type helper for detailed type information */
        return TypeHelper<
            std::is_signed<std::decay_t<Item>>::value,
            std::is_unsigned<std::decay_t<Item>>::value,
            std::is_convertible<std::decay_t<Item> *, Serializable *>::value,
            Item
        >::type(
            std::is_lvalue_reference<Item>::value &&
           !std::is_const<std::remove_reference<Item>>::value
        );
    }
};

#pragma clang diagnostic pop

/* void type */
template <> struct TypeItem<void> { static Type type(void) { return Type(); } };

/* single precision floating point number */
template <> struct TypeItem<      float  > { static Type type(void) { return Type(Type::TypeCode::Float, false); } };
template <> struct TypeItem<      float &> { static Type type(void) { return Type(Type::TypeCode::Float, true ); } };
template <> struct TypeItem<const float &> { static Type type(void) { return Type(Type::TypeCode::Float, false); } };

/* double precision floating point number */
template <> struct TypeItem<      double  > { static Type type(void) { return Type(Type::TypeCode::Double, false); } };
template <> struct TypeItem<      double &> { static Type type(void) { return Type(Type::TypeCode::Double, true ); } };
template <> struct TypeItem<const double &> { static Type type(void) { return Type(Type::TypeCode::Double, false); } };

/* boolean */
template <> struct TypeItem<      bool  > { static Type type(void) { return Type(Type::TypeCode::Boolean, false); } };
template <> struct TypeItem<      bool &> { static Type type(void) { return Type(Type::TypeCode::Boolean, true ); } };
template <> struct TypeItem<const bool &> { static Type type(void) { return Type(Type::TypeCode::Boolean, false); } };

/* STL string */
template <> struct TypeItem<      std::string  > { static Type type(void) { return Type(Type::TypeCode::String, false); } };
template <> struct TypeItem<      std::string &> { static Type type(void) { return Type(Type::TypeCode::String, true ); } };
template <> struct TypeItem<const std::string &> { static Type type(void) { return Type(Type::TypeCode::String, false); } };

/* STL vector (arrays) */
template <typename Item> struct TypeItem<      std::vector<Item>  > { static Type type(void) { return Type(Type::TypeCode::Array, TypeItem<Item>::type(), false); }};
template <typename Item> struct TypeItem<      std::vector<Item> &> { static Type type(void) { return Type(Type::TypeCode::Array, TypeItem<Item>::type(), true ); }};
template <typename Item> struct TypeItem<const std::vector<Item> &> { static Type type(void) { return Type(Type::TypeCode::Array, TypeItem<Item>::type(), false); }};

/****** Type array resolvers ******/

template <typename ... Items>
struct TypeArray;

template <typename Item, typename ... Items>
struct TypeArray<Item, Items ...>
{
    static void resolve(std::vector<Type> &types)
    {
        types.push_back(TypeItem<Item>::type());
        TypeArray<Items ...>::resolve(types);
    }
};

template <>
struct TypeArray<>
{
    static void resolve(std::vector<Type> &)
    {
        /* final recursion, no arguments left */
        /* thus nothing to do */
    }
};

/****** Method signature resolvers ******/

template <typename ... Args>
struct MetaArgs
{
    static std::vector<Type> type(void)
    {
        std::vector<Type> types;
        types.reserve(sizeof ... (Args));
        TypeArray<Args ...>::resolve(types);
        return std::move(types);
    }
};

template <typename R, typename ... Args>
struct MetaMethod
{
    Type result;
    std::string name;
    std::string signature;
    std::vector<Type> args;

public:
    explicit MetaMethod(const char *name) :
        name(name), args(MetaArgs<Args ...>::type()), result(TypeItem<R>::type())
    {
        signature = name;
        signature += "(";

        for (const auto type : args)
            signature += type.toSignature();

        signature += ")";
        signature += TypeItem<R>::type().toSignature();
    }
};

/****** Utilities ******/

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCSimplifyInspection"

template <typename T> struct IsVector                 : public std::false_type {};
template <typename T> struct IsVector<std::vector<T>> : public std::true_type  { typedef T ItemType; };

template <typename T>
struct IsObjectReference
{
    static const bool value =
        std::is_lvalue_reference<T>::value &&
       !std::is_const<std::remove_reference_t<T>>::value &&
        std::is_convertible<std::decay_t<T> *, Serializable *>::value;
};

template <typename T, typename Integer>
struct IsSignedIntegerLike
{
    static const bool value =
       !std::is_same    <std::decay_t<T>, bool>::value &&
        std::is_signed  <std::decay_t<T>      >::value &&
        std::is_integral<std::decay_t<T>      >::value &&
        (sizeof(std::decay_t<T>) == sizeof(Integer));
};

template <typename T, typename Integer>
struct IsUnsignedIntegerLike
{
    static const bool value =
       !std::is_same    <std::decay_t<T>, bool>::value &&
        std::is_unsigned<std::decay_t<T>      >::value &&
        std::is_integral<std::decay_t<T>      >::value &&
        (sizeof(std::decay_t<T>) == sizeof(Integer));
};

#pragma clang diagnostic pop
}
}

#endif /* SIMPLERPC_TYPEINFO_H */
