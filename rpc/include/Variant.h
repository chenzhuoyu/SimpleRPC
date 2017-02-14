/* Serializable/deserializable variant */

#ifndef SIMPLERPC_VARIANT_H
#define SIMPLERPC_VARIANT_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

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
    /* specializations below */
    template <typename T> inline       T &as(void);
    template <typename T> inline const T &as(void) const;

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
};

/** mutable specializations **/

/* signed integers */
template <> inline int8_t  &Variant::as<int8_t >(void) { return _s8 ; }
template <> inline int16_t &Variant::as<int16_t>(void) { return _s16; }
template <> inline int32_t &Variant::as<int32_t>(void) { return _s32; }
template <> inline int64_t &Variant::as<int64_t>(void) { return _s64; }

/* unsigned integers */
template <> inline uint8_t  &Variant::as<uint8_t >(void) { return _u8 ; }
template <> inline uint16_t &Variant::as<uint16_t>(void) { return _u16; }
template <> inline uint32_t &Variant::as<uint32_t>(void) { return _u32; }
template <> inline uint64_t &Variant::as<uint64_t>(void) { return _u64; }

/* floating point numbers */
template <> inline float  &Variant::as<float >(void) { return _float ; }
template <> inline double &Variant::as<double>(void) { return _double; }

/* STL string */
template <> inline std::string &Variant::as<std::string >(void) { return _string; }

/* compond types */
template <> inline Variant::Array  &Variant::as<Variant::Array >(void) { return _array ; }
template <> inline Variant::Object &Variant::as<Variant::Object>(void) { return _object; }

/** immutable specializations **/

/* signed integers */
template <> inline const int8_t  &Variant::as<int8_t >(void) const { return _s8 ; }
template <> inline const int16_t &Variant::as<int16_t>(void) const { return _s16; }
template <> inline const int32_t &Variant::as<int32_t>(void) const { return _s32; }
template <> inline const int64_t &Variant::as<int64_t>(void) const { return _s64; }

/* unsigned integers */
template <> inline const uint8_t  &Variant::as<uint8_t >(void) const { return _u8 ; }
template <> inline const uint16_t &Variant::as<uint16_t>(void) const { return _u16; }
template <> inline const uint32_t &Variant::as<uint32_t>(void) const { return _u32; }
template <> inline const uint64_t &Variant::as<uint64_t>(void) const { return _u64; }

/* floating point numbers */
template <> inline const float  &Variant::as<float >(void) const { return _float ; }
template <> inline const double &Variant::as<double>(void) const { return _double; }

/* STL string */
template <> inline const std::string &Variant::as<std::string >(void) const { return _string; }

/* compond types */
template <> inline const Variant::Array  &Variant::as<Variant::Array >(void) const { return _array ; }
template <> inline const Variant::Object &Variant::as<Variant::Object>(void) const { return _object; }
}
}

#endif /* SIMPLERPC_VARIANT_H */
