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
    template <bool isSigned, bool isUnsigned, typename T>
    inline T getIntegers(void) const
    {
        /* these `if` statement would be optimized away by compiler */
        if (isSigned)   return getSigned<sizeof(T), T>();
        if (isUnsigned) return getUnsigned<sizeof(T), T>();

        /* should never reaches here */
        static_assert(isSigned || isUnsigned, "Cannot convert to arbitrary types");
        abort();
    }

public:
    template <typename T>
    inline T get(void) const
    {
        /* signed or unsigned integers */
        return getIntegers<
            std::is_signed<T>::value,
            std::is_unsigned<T>::value,
            T
        >();
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

/* STL string, reference version */
template <>
inline const std::string &Variant::get<const std::string &>(void) const
{
    if (_type == Type::TypeCode::String)
        return _string;
    else
        throw Exceptions::TypeError(toString() + " is not a string");
}

/* compond types */
template <>
inline const Variant::Array  &Variant::get<const Variant::Array  &>(void) const
{
    if (_type == Type::TypeCode::Array)
        return _array;
    else
        throw Exceptions::TypeError(toString() + " is not an array");
}

template <>
inline const Variant::Object &Variant::get<const Variant::Object &>(void) const
{
    if (_type == Type::TypeCode::Struct)
        return _object;
    else
        throw Exceptions::TypeError(toString() + " is not an object");
}

/* STL string, value version */
template <> inline std::string Variant::get<std::string>(void) const { return get<const std::string &>(); }

/* compond types, value version */
template <> inline Variant::Array  Variant::get<Variant::Array >(void) const { return get<const Variant::Array  &>(); }
template <> inline Variant::Object Variant::get<Variant::Object>(void) const { return get<const Variant::Object &>(); }
}
}

#endif /* SIMPLERPC_VARIANT_H */
