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
class Type
{
    std::string _className;
    std::shared_ptr<Type> _itemType;

public:
    enum class TypeCode : int
    {
        Int8    = 0,
        Int16   = 1,
        Int32   = 2,
        Int64   = 3,

        UInt8   = 4,
        UInt16  = 5,
        UInt32  = 6,
        UInt64  = 7,

        Float   = 8,
        Double  = 9,

        Array   = 10,
        Struct  = 11,
        String  = 12,
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
        }
    }
};

class Field
{
    Type _type;
    bool _required;

private:
    size_t _offset;
    std::string _name;

public:
    explicit Field(const std::string &name, const Type &type, size_t offset, bool required) : _type(type), _name(name), _offset(offset), _required(required) {}

public:
    size_t offset(void) const { return _offset; }
    std::string name(void) const { return _name; }

public:
    const Type &type(void) const { return _type; }

public:
    bool isRequired(void) const { return _required; }

};

struct Struct
{
    struct Meta
    {
        typedef Struct *(* Constructor)(void);
        typedef std::unordered_map<std::string, Field> FieldData;

    public:
        FieldData fields;
        Constructor constructor;

    public:
        explicit Meta(const Constructor &constructor) : constructor(constructor) {}

    };

private:
    mutable Meta *_meta;
    mutable std::mutex _mutex;

private:
    std::string _name;
    std::unordered_set<std::string> _names;

protected:
    explicit Struct() : _meta(nullptr) {}

public:
    bool isSet(const std::string &name) const { return _names.find(name) != _names.end(); }

public:
    const Meta &meta(void) const;
    const std::string &name(void) const { return _name; }

protected:
    void setName(const std::string &name) { _name = name; }
    void setField(const std::string &name) { _names.insert(name); }

public:
    std::string readableName(void) const
    {
        int status;
        char *demangled = abi::__cxa_demangle(_name.c_str(), nullptr, nullptr, &status);
        std::string result = demangled ? std::string(demangled) : _name;

        free(demangled);
        return result;
    }
};

struct Registry
{
    static void addClass(const std::string &name, const Struct::Meta &meta);
    static Struct::Meta &findClass(const std::string &name);
};

template <typename T>
struct Descriptor
{
    struct FieldInfo
    {
        Type type;
        bool required;
        size_t offset;
        std::string name;

    public:
        explicit FieldInfo(const std::string &name, const Type &type, size_t offset, bool required) : name(name), type(type), offset(offset), required(required) {}

    };

public:
    explicit Descriptor(const std::vector<FieldInfo> &fields)
    {
        /* class meta data */
        Struct::Meta meta([](void) -> Struct *
        {
            /* just instaniate corresponding class */
            return new T;
        });

        /* fill fields data */
        for (const FieldInfo &info : fields)
        {
            meta.fields.insert({
                info.name,
                Field(
                    info.name,
                    info.type,
                    info.offset,
                    info.required
                )
            });
        }

        /* register to class registry */
        Registry::addClass(typeid(T).name(), meta);
    }
};

/* structs */
template <typename T>
inline Type resolve(const T &)
{
    static_assert(std::is_convertible<T *, Struct *>::value, "Cannot serialize or deserialize arbitrary type");
    return Type(Type::TypeCode::Struct, typeid(T).name());
}

/* arrays */
template <typename T>
inline Type resolve(const std::vector<T> &)
{
    /* `Type::resolve` won't actually use the value, so `nullptr` would be safe here */
    return Type(Type::TypeCode::Array, resolve(*(T *)nullptr));
}

/* signed integers */
template <> inline Type resolve<int8_t >(const int8_t  &) { return Type(Type::TypeCode::Int8 ); }
template <> inline Type resolve<int16_t>(const int16_t &) { return Type(Type::TypeCode::Int16); }
template <> inline Type resolve<int32_t>(const int32_t &) { return Type(Type::TypeCode::Int32); }
template <> inline Type resolve<int64_t>(const int64_t &) { return Type(Type::TypeCode::Int64); }

/* unsigned integers */
template <> inline Type resolve<uint8_t >(const uint8_t  &) { return Type(Type::TypeCode::UInt8 ); }
template <> inline Type resolve<uint16_t>(const uint16_t &) { return Type(Type::TypeCode::UInt16); }
template <> inline Type resolve<uint32_t>(const uint32_t &) { return Type(Type::TypeCode::UInt32); }
template <> inline Type resolve<uint64_t>(const uint64_t &) { return Type(Type::TypeCode::UInt64); }

/* float numbers */
template <> inline Type resolve<float >(const float  &) { return Type(Type::TypeCode::Float ); }
template <> inline Type resolve<double>(const double &) { return Type(Type::TypeCode::Double); }

/* string */
template <> inline Type resolve<std::string>(const std::string &) { return Type(Type::TypeCode::String); }
}

#endif /* SIMPLERPC_TYPETRAITS_H */
