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

#include "TypeInfo.h"
#include "Exceptions.h"
#include "Functional.h"

namespace SimpleRPC
{
namespace Internal
{
class Variant
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

        float    _float;
        double   _double;

        /* used by copy constructor and move constructor / assignment */
        uint8_t  _buffer[Functional::max(
            sizeof(int8_t  ),
            sizeof(int16_t ),
            sizeof(int32_t ),
            sizeof(int64_t ),
            sizeof(uint8_t ),
            sizeof(uint16_t),
            sizeof(uint32_t),
            sizeof(uint64_t),
            sizeof(float   ),
            sizeof(double  )
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
    Variant(const char        *value) : _type(Type::TypeCode::String), _string(value) {}
    Variant(const std::string &value) : _type(Type::TypeCode::String), _string(value) {}

public:
    Variant(const Array  &value) : _type(Type::TypeCode::Array ), _array (value) {}
    Variant(const Object &value) : _type(Type::TypeCode::Struct), _object(value) {}

public:
    Variant(Variant &&other)      { swap(std::move(other)); }
    Variant(const Variant &other) { assign(other);          }

public:
    Variant &operator=(Variant &&other)
    {
        swap(std::move(other));
        return *this;
    }

public:
    Variant &operator=(const Variant &other)
    {
        assign(other);
        return *this;
    }

public:
    void swap(Variant &&other)
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

public:
    template <typename T> inline    T get(void) const;
    template <typename T> inline void set(const T &value) { assign(Variant(value)); }

private:
    template <typename R, typename T, typename BL, typename BU>
    inline R rangeCheckF(const std::string &name, T value, BL min, BU max) const
    {
        if (value > max)
            throw Exceptions::ValueError("Float overflow [" + name + "]: " + toString());
        else if (value < min)
            throw Exceptions::ValueError("Float underflow [" + name + "]: " + toString());
        else
            return static_cast<R>(value);
    }

private:
    template <typename R, typename T, typename BL, typename BU>
    inline R rangeCheckI(const std::string &name, T value, BL min, BU max) const
    {
        if (value > max)
            throw Exceptions::ValueError("Integer overflow [" + name + "]: " + toString());
        else if (value < min)
            throw Exceptions::ValueError("Integer underflow [" + name + "]: " + toString());
        else
            return static_cast<R>(value);
    }

private:
    template <typename R, typename T, typename BL, typename BU>
    inline R rangeCheckFI(const std::string &name, T value, BL min, BU max) const
    {
        if (value > max)
            throw Exceptions::ValueError("Integer overflow [" + name + "]: " + toString());
        else if (value < min)
            throw Exceptions::ValueError("Integer underflow [" + name + "]: " + toString());
        else if (static_cast<R>(value) != value)
            throw Exceptions::ValueError("Number is not integer [" + name + "]: " + toString());
        else
            return static_cast<R>(value);
    }

public:
    Variant &operator[](ssize_t index)
    {
        if (index < 0 || index >= _array.size())
            return *_array[index].get();
        else
            throw Exceptions::IndexError(index);
    }

public:
    const Variant &operator[](ssize_t index) const
    {
        if (index >= 0 && index < _array.size())
            return *_array[index].get();
        else
            throw Exceptions::IndexError(index);
    }

public:
    Variant &operator[](const std::string &key)
    {
        if (_object.find(key) == _object.end())
            _object.insert({ key, std::make_shared<Variant>(0) });

        /* guaranted we have this key */
        return *_object.at(key);
    }

public:
    const Variant &operator[](const std::string &key) const
    {
        if (_object.find(key) != _object.end())
            return *_object.at(key);
        else
            throw Exceptions::NameError(key);
    }

private:
    template <typename ... Args>
    struct ArrayBuilder;

private:
    template <typename Arg, typename ... Args>
    struct ArrayBuilder<Arg, Args ...>
    {
        static std::vector<std::shared_ptr<Variant>> &build(std::vector<std::shared_ptr<Variant>> &array, const Arg &arg, const Args & ... args)
        {
            array.push_back(std::make_shared<Variant>(arg));
            return ArrayBuilder<Args ...>::build(array, args ...);
        }
    };

private:
    template <typename Arg>
    struct ArrayBuilder<Arg>
    {
        static std::vector<std::shared_ptr<Variant>> &build(std::vector<std::shared_ptr<Variant>> &array, const Arg &arg)
        {
            /* last recursion, only one argument left */
            array.push_back(std::make_shared<Variant>(arg));
            return array;
        }
    };

public:
    template <typename ... Args>
    static Variant array(const Args & ... args)
    {
        std::vector<std::shared_ptr<Variant>> array;
        return Variant(ArrayBuilder<Args ...>::build(array, args ...));
    }

#define cache_def()             \
    int cp = 0;                 \
    char cache[256] = {0};

#define cache_char(c)           \
    do                          \
    {                           \
        cache[cp] = (c);        \
        if (++cp == 255)        \
            cache_flush();      \
    } while (0)

#define cache_flush()           \
    do                          \
    {                           \
        if (cp)                 \
        {                       \
            cache[cp] = 0;      \
            result += cache;    \
            cp = 0;             \
        }                       \
    } while (0)

public:
    static std::string escapeString(const std::string &s)
    {
        size_t n = s.size();
        const char *p = s.data();

        cache_def();
        std::string result = "\"";

        while (n--)
        {
            char ch = *p++;

            if (ch == '\"')
            {
                cache_char('\\');
                cache_char(ch);
                continue;
            }

            switch (ch)
            {
                case '\\':
                {
                    cache_char('\\');
                    cache_char('\\');
                    break;
                }

                case '\t':
                {
                    cache_char('\\');
                    cache_char('t');
                    break;
                }

                case '\n':
                {
                    cache_char('\\');
                    cache_char('n');
                    break;
                }

                case '\r':
                {
                    cache_char('\\');
                    cache_char('r');
                    break;
                }

                default:
                {
                    if (ch >= ' ' && ch < 0x7f)
                    {
                        cache_char(ch);
                        break;
                    }

    #define to_hex(n)   (((n) < 10) ? ((n) + '0') : ((n) - 10 + 'a'))

                    cache_char('\\');
                    cache_char('x');
                    cache_char(to_hex(((ch & 0xF0) >> 4)));
                    cache_char(to_hex(((ch & 0x0F) >> 0)));
                    break;

    #undef to_hex
                }
            }
        }

        cache_char('\"');
        cache_flush();
        return result;
    }

#undef cache_def
#undef cache_char
#undef cache_flush

public:
    std::string toString(void) const
    {
        switch (_type)
        {
            /* signed integers */
            case Type::TypeCode::Int8   : return "int8_t("  + std::to_string(_s8 ) + ")";
            case Type::TypeCode::Int16  : return "int16_t(" + std::to_string(_s16) + ")";
            case Type::TypeCode::Int32  : return "int32_t(" + std::to_string(_s32) + ")";
            case Type::TypeCode::Int64  : return "int64_t(" + std::to_string(_s64) + ")";

            /* unsigned integers */
            case Type::TypeCode::UInt8  : return "uint8_t("  + std::to_string(_u8 ) + ")";
            case Type::TypeCode::UInt16 : return "uint16_t(" + std::to_string(_u16) + ")";
            case Type::TypeCode::UInt32 : return "uint32_t(" + std::to_string(_u32) + ")";
            case Type::TypeCode::UInt64 : return "uint64_t(" + std::to_string(_u64) + ")";

            /* floating point numbers */
            case Type::TypeCode::Float  : return "float("  + std::to_string(_float ) + ")";
            case Type::TypeCode::Double : return "double(" + std::to_string(_double) + ")";

            /* STL string */
            case Type::TypeCode::String : return Variant::escapeString(_string);

            /* arrays */
            case Type::TypeCode::Array:
            {
                std::ostringstream oss;
                std::vector<std::string> items;

                for (const auto &item : _array)
                    items.push_back(item->toString());

                std::copy(items.begin(), items.end(), std::ostream_iterator<std::string>(oss, ", "));
                return "[" + oss.str() + "]";
            }

            /* structs */
            case Type::TypeCode::Struct:
            {
                std::ostringstream oss;
                std::vector<std::string> items;

                for (const auto &item : _object)
                    items.push_back(Variant::escapeString(item.first) + ": " + item.second->toString());

                std::copy(items.begin(), items.end(), std::ostream_iterator<std::string>(oss, ", "));
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

/* signed integers */
template <>
inline int8_t Variant::get<int8_t>(void) const
{
    switch (_type)
    {
        case Type::TypeCode::Int8   : return _s8;
        case Type::TypeCode::Int16  : return rangeCheckI<int8_t>("int8_t", _s16, INT8_MIN, INT8_MAX);
        case Type::TypeCode::Int32  : return rangeCheckI<int8_t>("int8_t", _s32, INT8_MIN, INT8_MAX);
        case Type::TypeCode::Int64  : return rangeCheckI<int8_t>("int8_t", _s64, INT8_MIN, INT8_MAX);

        case Type::TypeCode::UInt8  : return rangeCheckI<int8_t>("int8_t", _u8 , INT8_MIN, INT8_MAX);
        case Type::TypeCode::UInt16 : return rangeCheckI<int8_t>("int8_t", _u16, INT8_MIN, INT8_MAX);
        case Type::TypeCode::UInt32 : return rangeCheckI<int8_t>("int8_t", _u32, INT8_MIN, INT8_MAX);
        case Type::TypeCode::UInt64 : return rangeCheckI<int8_t>("int8_t", _u64, INT8_MIN, INT8_MAX);

        case Type::TypeCode::Float  : return rangeCheckFI<int8_t>("int8_t", _float , INT8_MIN, INT8_MAX);
        case Type::TypeCode::Double : return rangeCheckFI<int8_t>("int8_t", _double, INT8_MIN, INT8_MAX);

        default:
            throw Exceptions::TypeError(toString() + " is not convertiable to int8_t");
    }
}

template <>
inline int16_t Variant::get<int16_t>(void) const
{
    switch (_type)
    {
        case Type::TypeCode::Int8   : return _s8;
        case Type::TypeCode::Int16  : return _s16;
        case Type::TypeCode::Int32  : return rangeCheckI<int16_t>("int16_t", _s32, INT16_MIN, INT16_MAX);
        case Type::TypeCode::Int64  : return rangeCheckI<int16_t>("int16_t", _s64, INT16_MIN, INT16_MAX);

        case Type::TypeCode::UInt8  : return rangeCheckI<int16_t>("int16_t", _u8 , INT16_MIN, INT16_MAX);
        case Type::TypeCode::UInt16 : return rangeCheckI<int16_t>("int16_t", _u16, INT16_MIN, INT16_MAX);
        case Type::TypeCode::UInt32 : return rangeCheckI<int16_t>("int16_t", _u32, INT16_MIN, INT16_MAX);
        case Type::TypeCode::UInt64 : return rangeCheckI<int16_t>("int16_t", _u64, INT16_MIN, INT16_MAX);

        case Type::TypeCode::Float  : return rangeCheckFI<int16_t>("int16_t", _float , INT16_MIN, INT16_MAX);
        case Type::TypeCode::Double : return rangeCheckFI<int16_t>("int16_t", _double, INT16_MIN, INT16_MAX);

        default:
            throw Exceptions::TypeError(toString() + " is not convertiable to int16_t");
    }
}

template <>
inline int32_t Variant::get<int32_t>(void) const
{
    switch (_type)
    {
        case Type::TypeCode::Int8   : return _s8;
        case Type::TypeCode::Int16  : return _s16;
        case Type::TypeCode::Int32  : return _s32;
        case Type::TypeCode::Int64  : return rangeCheckI<int32_t>("int32_t", _s64, INT32_MIN, INT32_MAX);

        case Type::TypeCode::UInt8  : return rangeCheckI<int32_t>("int32_t", _u8 , INT32_MIN, INT32_MAX);
        case Type::TypeCode::UInt16 : return rangeCheckI<int32_t>("int32_t", _u16, INT32_MIN, INT32_MAX);
        case Type::TypeCode::UInt32 : return rangeCheckI<int32_t>("int32_t", _u32, INT32_MIN, INT32_MAX);
        case Type::TypeCode::UInt64 : return rangeCheckI<int32_t>("int32_t", _u64, INT32_MIN, INT32_MAX);

        case Type::TypeCode::Float  : return rangeCheckFI<int32_t>("int32_t", _float , INT32_MIN, INT32_MAX);
        case Type::TypeCode::Double : return rangeCheckFI<int32_t>("int32_t", _double, INT32_MIN, INT32_MAX);

        default:
            throw Exceptions::TypeError(toString() + " is not convertiable to int32_t");
    }
}

template <>
inline int64_t Variant::get<int64_t>(void) const
{
    switch (_type)
    {
        case Type::TypeCode::Int8   : return _s8;
        case Type::TypeCode::Int16  : return _s16;
        case Type::TypeCode::Int32  : return _s32;
        case Type::TypeCode::Int64  : return _s64;

        case Type::TypeCode::UInt8  : return rangeCheckI<int64_t>("int64_t", _u8 , INT64_MIN, INT64_MAX);
        case Type::TypeCode::UInt16 : return rangeCheckI<int64_t>("int64_t", _u16, INT64_MIN, INT64_MAX);
        case Type::TypeCode::UInt32 : return rangeCheckI<int64_t>("int64_t", _u32, INT64_MIN, INT64_MAX);
        case Type::TypeCode::UInt64 : return rangeCheckI<int64_t>("int64_t", _u64, INT64_MIN, INT64_MAX);

        case Type::TypeCode::Float  : return rangeCheckFI<int64_t>("int64_t", _float , INT64_MIN, INT64_MAX);
        case Type::TypeCode::Double : return rangeCheckFI<int64_t>("int64_t", _double, INT64_MIN, INT64_MAX);

        default:
            throw Exceptions::TypeError(toString() + " is not convertiable to int64_t");
    }
}

/* unsigned integers */
template <>
inline uint8_t Variant::get<uint8_t>(void) const
{
    switch (_type)
    {
        case Type::TypeCode::Int8   : return rangeCheckI<uint8_t>("uint8_t", _s8 , 0, UINT8_MAX);
        case Type::TypeCode::Int16  : return rangeCheckI<uint8_t>("uint8_t", _s16, 0, UINT8_MAX);
        case Type::TypeCode::Int32  : return rangeCheckI<uint8_t>("uint8_t", _s32, 0, UINT8_MAX);
        case Type::TypeCode::Int64  : return rangeCheckI<uint8_t>("uint8_t", _s64, 0, UINT8_MAX);

        case Type::TypeCode::UInt8  : return _u8;
        case Type::TypeCode::UInt16 : return rangeCheckI<uint8_t>("uint8_t", _u16, 0, UINT8_MAX);
        case Type::TypeCode::UInt32 : return rangeCheckI<uint8_t>("uint8_t", _u32, 0, UINT8_MAX);
        case Type::TypeCode::UInt64 : return rangeCheckI<uint8_t>("uint8_t", _u64, 0, UINT8_MAX);

        case Type::TypeCode::Float  : return rangeCheckFI<uint8_t>("uint8_t", _float , 0, UINT8_MAX);
        case Type::TypeCode::Double : return rangeCheckFI<uint8_t>("uint8_t", _double, 0, UINT8_MAX);

        default:
            throw Exceptions::TypeError(toString() + " is not convertiable to uint8_t");
    }
}

template <>
inline uint16_t Variant::get<uint16_t>(void) const
{
    switch (_type)
    {
        case Type::TypeCode::Int8   : return rangeCheckI<uint16_t>("uint16_t", _s8 , 0, UINT16_MAX);
        case Type::TypeCode::Int16  : return rangeCheckI<uint16_t>("uint16_t", _s16, 0, UINT16_MAX);
        case Type::TypeCode::Int32  : return rangeCheckI<uint16_t>("uint16_t", _s32, 0, UINT16_MAX);
        case Type::TypeCode::Int64  : return rangeCheckI<uint16_t>("uint16_t", _s64, 0, UINT16_MAX);

        case Type::TypeCode::UInt8  : return _u8;
        case Type::TypeCode::UInt16 : return _u16;
        case Type::TypeCode::UInt32 : return rangeCheckI<uint16_t>("uint16_t", _u32, 0, UINT16_MAX);
        case Type::TypeCode::UInt64 : return rangeCheckI<uint16_t>("uint16_t", _u64, 0, UINT16_MAX);

        case Type::TypeCode::Float  : return rangeCheckFI<uint16_t>("uint16_t", _float , 0, UINT16_MAX);
        case Type::TypeCode::Double : return rangeCheckFI<uint16_t>("uint16_t", _double, 0, UINT16_MAX);

        default:
            throw Exceptions::TypeError(toString() + " is not convertiable to uint16_t");
    }
}

template <>
inline uint32_t Variant::get<uint32_t>(void) const
{
    switch (_type)
    {
        case Type::TypeCode::Int8   : return rangeCheckI<uint32_t>("uint32_t", _s8 , 0, UINT32_MAX);
        case Type::TypeCode::Int16  : return rangeCheckI<uint32_t>("uint32_t", _s16, 0, UINT32_MAX);
        case Type::TypeCode::Int32  : return rangeCheckI<uint32_t>("uint32_t", _s32, 0, UINT32_MAX);
        case Type::TypeCode::Int64  : return rangeCheckI<uint32_t>("uint32_t", _s64, 0, UINT32_MAX);

        case Type::TypeCode::UInt8  : return _u8;
        case Type::TypeCode::UInt16 : return _u16;
        case Type::TypeCode::UInt32 : return _u32;
        case Type::TypeCode::UInt64 : return rangeCheckI<uint32_t>("uint32_t", _u64, 0, UINT32_MAX);

        case Type::TypeCode::Float  : return rangeCheckFI<uint32_t>("uint32_t", _float , 0, UINT32_MAX);
        case Type::TypeCode::Double : return rangeCheckFI<uint32_t>("uint32_t", _double, 0, UINT32_MAX);

        default:
            throw Exceptions::TypeError(toString() + " is not convertiable to uint32_t");
    }
}

template <>
inline uint64_t Variant::get<uint64_t>(void) const
{
    switch (_type)
    {
        case Type::TypeCode::Int8   : return rangeCheckI<uint64_t>("uint64_t", _s8 , 0, UINT64_MAX);
        case Type::TypeCode::Int16  : return rangeCheckI<uint64_t>("uint64_t", _s16, 0, UINT64_MAX);
        case Type::TypeCode::Int32  : return rangeCheckI<uint64_t>("uint64_t", _s32, 0, UINT64_MAX);
        case Type::TypeCode::Int64  : return rangeCheckI<uint64_t>("uint64_t", _s64, 0, UINT64_MAX);

        case Type::TypeCode::UInt8  : return _u8;
        case Type::TypeCode::UInt16 : return _u16;
        case Type::TypeCode::UInt32 : return _u32;
        case Type::TypeCode::UInt64 : return _u64;

        case Type::TypeCode::Float  : return rangeCheckFI<uint64_t>("uint64_t", _float , 0, UINT64_MAX);
        case Type::TypeCode::Double : return rangeCheckFI<uint64_t>("uint64_t", _double, 0, UINT64_MAX);

        default:
            throw Exceptions::TypeError(toString() + " is not convertiable to uint64_t");
    }
}

/* floating point numbers */
template <>
inline float Variant::get<float>(void) const
{
    switch (_type)
    {
        case Type::TypeCode::Int8   : return _s8;
        case Type::TypeCode::Int16  : return _s16;
        case Type::TypeCode::Int32  : return _s32;
        case Type::TypeCode::Int64  : return _s64;

        case Type::TypeCode::UInt8  : return _u8;
        case Type::TypeCode::UInt16 : return _u16;
        case Type::TypeCode::UInt32 : return _u32;
        case Type::TypeCode::UInt64 : return _u64;

        case Type::TypeCode::Float  : return _float;
        case Type::TypeCode::Double : return rangeCheckF<float>("float", _double, FLT_MIN, FLT_MAX);

        default:
            throw Exceptions::TypeError(toString() + " is not convertiable to float");
    }
}

template <>
inline double Variant::get<double>(void) const
{
    switch (_type)
    {
        case Type::TypeCode::Int8   : return _s8;
        case Type::TypeCode::Int16  : return _s16;
        case Type::TypeCode::Int32  : return _s32;
        case Type::TypeCode::Int64  : return _s64;

        case Type::TypeCode::UInt8  : return _u8;
        case Type::TypeCode::UInt16 : return _u16;
        case Type::TypeCode::UInt32 : return _u32;
        case Type::TypeCode::UInt64 : return _u64;

        case Type::TypeCode::Float  : return _float;
        case Type::TypeCode::Double : return _double;

        default:
            throw Exceptions::TypeError(toString() + " is not convertiable to double");
    }
}

/* STL string, reference version */
template <>
inline const std::string &Variant::get<const std::string &>(void) const
{
    if (_type == Type::TypeCode::String)
        return _string;
    else
        throw Exceptions::TypeError(toString() + " is not convertiable to string");
}

/* compond types */
template <>
inline const Variant::Array  &Variant::get<const Variant::Array  &>(void) const
{
    if (_type == Type::TypeCode::Array)
        return _array;
    else
        throw Exceptions::TypeError(toString() + " is not convertiable to array");
}

template <>
inline const Variant::Object &Variant::get<const Variant::Object &>(void) const
{
    if (_type == Type::TypeCode::Struct)
        return _object;
    else
        throw Exceptions::TypeError(toString() + " is not convertiable to object");
}

/* STL string, value version */
template <> inline std::string Variant::get<std::string>(void) const { return get<const std::string &>(); }

/* compond types, value version */
template <> inline Variant::Array  Variant::get<Variant::Array >(void) const { return get<const Variant::Array  &>(); }
template <> inline Variant::Object Variant::get<Variant::Object>(void) const { return get<const Variant::Object &>(); }
}
}

#endif /* SIMPLERPC_VARIANT_H */
