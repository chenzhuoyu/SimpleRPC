/* Serializable/deserializable variant */

#ifndef SIMPLERPC_VARIANT_H
#define SIMPLERPC_VARIANT_H

#include <string>
#include <memory>
#include <vector>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <unordered_map>

#include <float.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "ByteSeq.h"
#include "TypeInfo.h"
#include "Registry.h"
#include "Exceptions.h"
#include "Functional.h"

namespace SimpleRPC
{
namespace Internal
{
class Variant final
{
    union
    {
        int8_t   _s8;
        int16_t  _s16;
        int32_t  _s32;
        int64_t  _s64;

        uint8_t  _u8;
        uint16_t _u16;
        uint32_t _u32;
        uint64_t _u64;

        bool     _bool;
        float    _float;
        double   _double;

        /* used by copy constructor and move constructor / assignment */
        uint8_t  _buffer[Functional::max(
            sizeof(int8_t   ),
            sizeof(int16_t  ),
            sizeof(int32_t  ),
            sizeof(int64_t  ),
            sizeof(uint8_t  ),
            sizeof(uint16_t ),
            sizeof(uint32_t ),
            sizeof(uint64_t ),
            sizeof(bool     ),
            sizeof(float    ),
            sizeof(double   )
        )];
    };

public:
    typedef std::vector<std::shared_ptr<Variant>> Array;
    typedef std::unordered_map<std::string, std::shared_ptr<Variant>> Object;

private:
    Array _array;
    Object _object;
    std::string _string;

private:
    Type::TypeCode _type;

public:
    explicit Variant(const Type::TypeCode &type) : _type(type) {}

public:
    Variant(int8_t  value) : _type(Type::TypeCode::Int8 ), _s8 (value) {}
    Variant(int16_t value) : _type(Type::TypeCode::Int16), _s16(value) {}
    Variant(int32_t value) : _type(Type::TypeCode::Int32), _s32(value) {}
    Variant(int64_t value) : _type(Type::TypeCode::Int64), _s64(value) {}

public:
    Variant(uint8_t  value) : _type(Type::TypeCode::UInt8 ), _u8 (value) {}
    Variant(uint16_t value) : _type(Type::TypeCode::UInt16), _u16(value) {}
    Variant(uint32_t value) : _type(Type::TypeCode::UInt32), _u32(value) {}
    Variant(uint64_t value) : _type(Type::TypeCode::UInt64), _u64(value) {}

public:
    Variant(float  value) : _type(Type::TypeCode::Float ), _float (value) {}
    Variant(double value) : _type(Type::TypeCode::Double), _double(value) {}

public:
    Variant(bool value) : _type(Type::TypeCode::Boolean), _bool(value) {}

public:
    Variant(const char        *value) : _type(Type::TypeCode::String), _string(value) {}
    Variant(const std::string &value) : _type(Type::TypeCode::String), _string(value) {}

public:
    template <typename T>
    Variant(const std::vector<T> &value) : _type(Type::TypeCode::Array)
    {
        for (const auto &item : value)
            _array.push_back(std::make_shared<Variant>(item));
    }

public:
    template <typename T, typename = std::enable_if_t<std::is_convertible<T *, Serializable *>::value, void>>
    Variant(const T &value) : _type(Type::TypeCode::Object) { assign(value.serialize()); }

public:
    Variant(Variant &&other)      { swap(other);   }
    Variant(const Variant &other) { assign(other); }

public:
    Variant &operator=(Variant &&other)
    {
        swap(other);
        return *this;
    }

public:
    Variant &operator=(const Variant &other)
    {
        assign(other);
        return *this;
    }

public:
    void swap(Variant &other)
    {
        std::swap(_type, other._type);
        std::swap(_array, other._array);
        std::swap(_object, other._object);
        std::swap(_string, other._string);

        /* no need to clear other's buffer, primitive types are harmless */
        memcpy(_buffer, other._buffer, sizeof(_buffer));
    }

public:
    void assign(const Variant &other)
    {
        _type = other._type;
        _array = other._array;
        _object = other._object;
        _string = other._string;

        /* copy from other union */
        memcpy(_buffer, other._buffer, sizeof(_buffer));
    }

public:
    Type::TypeCode type(void) const { return _type; }

private:
    template <size_t size, typename T>
    inline T getSigned(void) const
    {
        /* these `if` statement would be optimized away by compiler since `size` is literally a constant */
        if (size == sizeof(int8_t))
        {
            if (_type == Type::TypeCode::Int8)
                return static_cast<T>(_s8);
            else
                throw Exceptions::TypeError(toString() + " is not an `int8_t`");
        }
        else if (size == sizeof(int16_t))
        {
            if (_type == Type::TypeCode::Int16)
                return static_cast<T>(_s16);
            else
                throw Exceptions::TypeError(toString() + " is not an `int16_t`");
        }
        else if (size == sizeof(int32_t))
        {
            if (_type == Type::TypeCode::Int32)
                return static_cast<T>(_s32);
            else
                throw Exceptions::TypeError(toString() + " is not an `int32_t`");
        }
        else if (size == sizeof(int64_t))
        {
            if (_type == Type::TypeCode::Int64)
                return static_cast<T>(_s64);
            else
                throw Exceptions::TypeError(toString() + " is not an `int64_t`");
        }
        else
        {
            /* should never reaches here */
            static_assert(size == sizeof(int8_t) || size == sizeof(int16_t) || size == sizeof(int32_t) || size == sizeof(int64_t), "Unknown signed integer size");
            abort();
        }
    }

private:
    template <size_t size, typename T>
    inline T getUnsigned(void) const
    {
        /* these `if` statement would be optimized away by compiler since `size` is literally a constant */
        if (size == sizeof(uint8_t))
        {
            if (_type == Type::TypeCode::UInt8)
                return static_cast<T>(_u8);
            else
                throw Exceptions::TypeError(toString() + " is not an `uint8_t`");
        }
        else if (size == sizeof(uint16_t))
        {
            if (_type == Type::TypeCode::UInt16)
                return static_cast<T>(_u16);
            else
                throw Exceptions::TypeError(toString() + " is not an `uint16_t`");
        }
        else if (size == sizeof(uint32_t))
        {
            if (_type == Type::TypeCode::UInt32)
                return static_cast<T>(_u32);
            else
                throw Exceptions::TypeError(toString() + " is not an `uint32_t`");
        }
        else if (size == sizeof(uint64_t))
        {
            if (_type == Type::TypeCode::UInt64)
                return static_cast<T>(_u64);
            else
                throw Exceptions::TypeError(toString() + " is not an `uint64_t`");
        }
        else
        {
            /* should never reaches here */
            static_assert(size == sizeof(uint8_t) || size == sizeof(uint16_t) || size == sizeof(uint32_t) || size == sizeof(uint64_t), "Unknown unsigned integer size");
            abort();
        }
    }

private:
    template <typename T> inline std::enable_if_t<std::is_signed  <T>::value, void> getValue(T &v) const { v = getSigned  <sizeof(T), T>(); }
    template <typename T> inline std::enable_if_t<std::is_unsigned<T>::value, void> getValue(T &v) const { v = getUnsigned<sizeof(T), T>(); }

private:
    template <typename T>
    inline std::enable_if_t<IsVector<T>::value, void> getValue(T &v) const
    {
        if (_type != Type::TypeCode::Array)
            throw Exceptions::TypeError(toString() + " is not an array");

        typedef typename IsVector<T>::ItemType ItemType;
        std::vector<ItemType> *array = static_cast<std::vector<ItemType> *>(&v);

        for (const auto &item : _array)
            array->push_back(item->get<ItemType>());
    }

private:
    template <typename T>
    inline std::enable_if_t<std::is_convertible<T *, Serializable *>::value, void> getValue(T &v) const
    {
        /* it's ensured to be convertible to `Serializable` as `std:enable_if_t<...>` says */
        if (_type == Type::TypeCode::Object)
            v.deserialize(*this);
        else
            throw Exceptions::TypeError(toString() + " is not an object");
    }

public:
    template <typename T>
    inline T get(void) const
    {
        T v;
        getValue(v);
        return std::move(v);
    }

public:
    size_t size(void) const
    {
        if (_type == Type::TypeCode::Array)
            return _array.size();
        else
            throw Exceptions::TypeError(toString() + " is not an array");
    }

public:
    std::vector<std::string> keys(void) const
    {
        if (_type != Type::TypeCode::Object)
            throw Exceptions::TypeError(toString() + " is not an object");

        std::vector<std::string> result(_object.size());
        std::transform(_object.begin(), _object.end(), result.begin(), [](auto x){ return x.first; });
        return result;
    }

public:
    Variant &operator[](ssize_t index)
    {
        if (_type != Type::TypeCode::Array)
            throw Exceptions::TypeError(toString() + " is not an array");
        else if (index < 0 || index >= _array.size())
            throw Exceptions::IndexError(index);
        else
            return *_array[index].get();
    }

public:
    const Variant &operator[](ssize_t index) const
    {
        if (_type != Type::TypeCode::Array)
            throw Exceptions::TypeError(toString() + " is not an array");
        else if (index < 0 || index >= _array.size())
            throw Exceptions::IndexError(index);
        else
            return *_array[index].get();
    }

public:
    Variant &operator[](const std::string &key)
    {
        if (_type != Type::TypeCode::Object)
            throw Exceptions::TypeError(toString() + " is not an object");
        else if (_object.find(key) == _object.end())
            _object.insert({ key, std::make_shared<Variant>(0) });

        /* guaranted we have this key */
        return *_object.at(key);
    }

public:
    const Variant &operator[](const std::string &key) const
    {
        if (_type != Type::TypeCode::Object)
            throw Exceptions::TypeError(toString() + " is not an object");
        else if (_object.find(key) == _object.end())
            throw Exceptions::NameError(key);
        else
            return *_object.at(key);
    }

/** BEGIN :: these methods should only be used by serialization / deserialization backends unless you know what you are doing **/

public:
    Array &internalArray(void) { return _array; }
    Object &internalObject(void) { return _object; }

public:
    const Array &internalArray(void) const { return _array; }
    const Object &internalObject(void) const { return _object; }

/** END **/

private:
    template <typename ... Args>
    struct ArrayBuilder;

private:
    template <typename Arg, typename ... Args>
    struct ArrayBuilder<Arg, Args ...>
    {
        static void build(Array &array, const Arg &arg, const Args & ... args)
        {
            array.push_back(std::make_shared<Variant>(arg));
            ArrayBuilder<Args ...>::build(array, args ...);
        }
    };

private:
    template <typename Arg>
    struct ArrayBuilder<Arg>
    {
        static void build(Array &array, const Arg &arg)
        {
            /* last recursion, only one argument left */
            array.push_back(std::make_shared<Variant>(arg));
        }
    };

public:
    template <typename ... Args>
    static Variant array(const Args & ... args)
    {
        Variant result(Type::TypeCode::Array);
        std::vector<std::shared_ptr<Variant>> array;
        ArrayBuilder<Args ...>::build(array, args ...);

        result._array = std::move(array);
        return result;
    }

public:
    struct VariantPair
    {
        std::string name;
        std::shared_ptr<Variant> value;

    public:
        template <typename T>
        VariantPair(const std::string &name, T &&value) : name(name), value(std::make_shared<Variant>(std::forward<T>(value))) {}

    };

public:
    static Variant object(std::initializer_list<VariantPair> list)
    {
        /* create object type */
        Variant result(Type::TypeCode::Object);

        /* fill each key-value pair */
        for (const auto &pair : list)
            result._object.insert({ pair.name, pair.value });

        return result;
    }

public:
    std::string toString(void) const
    {
        switch (_type)
        {
            /* signed integers */
            case Type::TypeCode::Int8       : return "int8_t("  + std::to_string(_s8 ) + ")";
            case Type::TypeCode::Int16      : return "int16_t(" + std::to_string(_s16) + ")";
            case Type::TypeCode::Int32      : return "int32_t(" + std::to_string(_s32) + ")";
            case Type::TypeCode::Int64      : return "int64_t(" + std::to_string(_s64) + ")";

            /* unsigned integers */
            case Type::TypeCode::UInt8      : return "uint8_t("  + std::to_string(_u8 ) + ")";
            case Type::TypeCode::UInt16     : return "uint16_t(" + std::to_string(_u16) + ")";
            case Type::TypeCode::UInt32     : return "uint32_t(" + std::to_string(_u32) + ")";
            case Type::TypeCode::UInt64     : return "uint64_t(" + std::to_string(_u64) + ")";

            /* floating point numbers */
            case Type::TypeCode::Float      : return "float("  + std::to_string(_float ) + ")";
            case Type::TypeCode::Double     : return "double(" + std::to_string(_double) + ")";

            /* boolean */
            case Type::TypeCode::Boolean    : return "boolean(" + std::string(_bool ? "true" : "false") + ")";

            /* STL string */
            case Type::TypeCode::String     : return ByteSeq::repr(_string);

            /* arrays */
            case Type::TypeCode::Array:
            {
                std::ostringstream oss;
                std::vector<std::string> items;

                for (const auto &item : _array)
                {
                    items.push_back(item->toString());
                    items.push_back(", ");
                }

                items.pop_back();
                std::copy(items.begin(), items.end(), std::ostream_iterator<std::string>(oss));
                return "[" + oss.str() + "]";
            }

            /* structs */
            case Type::TypeCode::Object:
            {
                std::ostringstream oss;
                std::vector<std::string> items;

                for (const auto &item : _object)
                {
                    items.push_back(ByteSeq::repr(item.first) + ": " + item.second->toString());
                    items.push_back(", ");
                }

                items.pop_back();
                std::copy(items.begin(), items.end(), std::ostream_iterator<std::string>(oss));
                return "{" + oss.str() + "}";
            }

            default:
            {
                fprintf(stderr, "*** FATAL: impossible type %d", (int)_type);
                abort();
            }
        }
    }
};

/** value getter specializations **/

/* floating point numbers */
template <>
inline float Variant::get<float>(void) const
{
    if (_type == Type::TypeCode::Float)
        return _float;
    else
        throw Exceptions::TypeError(toString() + " is not a float");
}

template <>
inline double Variant::get<double>(void) const
{
    if (_type == Type::TypeCode::Double)
        return _double;
    else
        throw Exceptions::TypeError(toString() + " is not a double");
}

/* boolean */

template <>
inline bool Variant::get<bool>(void) const
{
    if (_type == Type::TypeCode::Boolean)
        return _bool;
    else
        throw Exceptions::TypeError(toString() + " is not a boolean");
}

/* STL string, reference version */
template <>
inline const std::string &Variant::get<const std::string &>(void) const
{
    if (_type == Type::TypeCode::String)
        return _string;
    else
        throw Exceptions::TypeError(toString() + " is not a string");
}

/* STL string, value version */
template <> inline std::string Variant::get<std::string>(void) const { return get<const std::string &>(); }
}
}

#endif /* SIMPLERPC_VARIANT_H */
