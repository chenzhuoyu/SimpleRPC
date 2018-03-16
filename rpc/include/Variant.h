/* Serializable/deserializable variant */

#ifndef SIMPLERPC_VARIANT_H
#define SIMPLERPC_VARIANT_H

#include <string>
#include <memory>
#include <vector>
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
class Variant;
struct VariantHashKey final
{
    std::shared_ptr<Variant> key;

public:
    VariantHashKey(Variant &&key) : key(std::make_shared<Variant>(key)) {}

public:
    bool operator==(const VariantHashKey &other) const;
    bool operator!=(const VariantHashKey &other) const { return !(*this == other); }

public:
    struct Hash { size_t operator()(const VariantHashKey &key) const; };

};

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
        uint8_t  _buffer[Internal::Functional::max(
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
    typedef std::unordered_map<VariantHashKey, std::shared_ptr<Variant>, VariantHashKey::Hash> Map;

private:
    Map _map;
    Array _array;
    Object _object;
    std::string _string;

public:
    enum class ArrayElementType
    {
        Int8,
        UInt8,
        Generic,
    };

private:
    Type::TypeCode _type = Type::TypeCode::Void;
    ArrayElementType _itemType = ArrayElementType::Generic;

public:
    explicit Variant() = default;
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
        /* reserve space to prevent frequent malloc */
        _array.reserve(value.size());

        /* append every item */
        for (const auto &item : value)
            _array.push_back(std::make_shared<Variant>(item));

        /* check for element type, these `if` statements will be optimized compile time */
        if (Internal::IsSignedIntegerLike<T, int8_t>::value)
            _itemType = ArrayElementType::Int8;
        else if (Internal::IsUnsignedIntegerLike<T, uint8_t>::value)
            _itemType = ArrayElementType::UInt8;
        else
            _itemType = ArrayElementType::Generic;
    }

public:
    template <typename K, typename V>
    Variant(const std::map<K, V> &value) : _type(Type::TypeCode::Map)
    {
        /* reserve space to prevent frequent malloc */
        _map.reserve(value.size());

        /* append every item */
        for (const auto &item : value)
        {
            _map.emplace(
                VariantHashKey(Variant(item.first)),
                std::make_shared<Variant>(item.second)
            );
        }
    }

public:
    template <typename K, typename V>
    Variant(const std::unordered_map<K, V> &value) : _type(Type::TypeCode::Map)
    {
        /* reserve space to prevent frequent malloc */
        _map.reserve(value.size());

        /* append every item */
        for (const auto &item : value)
        {
            _map.emplace(
                VariantHashKey(Variant(item.first)),
                std::make_shared<Variant>(item.second)
            );
        }
    }

public:
    template <typename T, typename = std::enable_if_t<std::is_convertible<T *, Serializable *>::value, void>>
    Variant(const T &value) : _type(Type::TypeCode::Object) { assign(value.serialize()); }

public:
    Variant(Variant &&other)      { swap(other);   }
    Variant(const Variant &other) { assign(other); }

public:
    Variant &operator=(Variant &&other)      { swap(other);   return *this; }
    Variant &operator=(const Variant &other) { assign(other); return *this; }

public:
    void swap(Variant &other)
    {
        std::swap(_map, other._map);
        std::swap(_type, other._type);
        std::swap(_array, other._array);
        std::swap(_object, other._object);
        std::swap(_string, other._string);

        /* copy from other's buffer, then clear it */
        memcpy(_buffer, other._buffer, sizeof(_buffer));
        memset(other._buffer, 0, sizeof(other._buffer));
    }

public:
    void assign(const Variant &other)
    {
        _map = other._map;
        _type = other._type;
        _array = other._array;
        _object = other._object;
        _string = other._string;

        /* copy from other union */
        memcpy(_buffer, other._buffer, sizeof(_buffer));
    }

public:
    Type::TypeCode type(void) const { return _type; }
    ArrayElementType arrayElementType(void) const
    {
        if (_type != Type::TypeCode::Array)
            throw Exceptions::TypeError(toString() + " is not an array");
        else
            return _itemType;
    }

private:
    static inline size_t hashCombine(size_t seed, size_t value)
    {
        value += 0x9e3779b9;
        value += seed << 6;
        value += seed >> 2;
        return seed ^ value;
    }

public:
    size_t hash(void) const
    {
        /* hash of the type */
        int type = static_cast<int>(_type);
        size_t hash = std::hash<int>()(type);

        /* hash the values */
        switch (_type)
        {
            case Type::TypeCode::Void    : return hashCombine(hash, 0);
            case Type::TypeCode::Int8    : return hashCombine(hash, std::hash<int8_t >()(_s8 ));
            case Type::TypeCode::Int16   : return hashCombine(hash, std::hash<int16_t>()(_s16));
            case Type::TypeCode::Int32   : return hashCombine(hash, std::hash<int32_t>()(_s32));
            case Type::TypeCode::Int64   : return hashCombine(hash, std::hash<int64_t>()(_s64));

            case Type::TypeCode::UInt8   : return hashCombine(hash, std::hash<uint8_t >()(_u8 ));
            case Type::TypeCode::UInt16  : return hashCombine(hash, std::hash<uint16_t>()(_u16));
            case Type::TypeCode::UInt32  : return hashCombine(hash, std::hash<uint32_t>()(_u32));
            case Type::TypeCode::UInt64  : return hashCombine(hash, std::hash<uint64_t>()(_u64));

            case Type::TypeCode::Float   : return hashCombine(hash, std::hash<float      >()(_float ));
            case Type::TypeCode::Double  : return hashCombine(hash, std::hash<double     >()(_double));
            case Type::TypeCode::String  : return hashCombine(hash, std::hash<std::string>()(_string));
            case Type::TypeCode::Boolean : return hashCombine(hash, std::hash<bool       >()(_bool  ));

            case Type::TypeCode::Map:
            {
                for (const auto &item : _map)
                {
                    hash = hashCombine(hash, item.second->hash());
                    hash = hashCombine(hash, item.first.key->hash());
                }

                return hash;
            }

            case Type::TypeCode::Array:
            {
                for (const auto &item : _array)
                    hash = hashCombine(hash, item->hash());

                return hash;
            }

            case Type::TypeCode::Object:
            {
                for (const auto &item : _object)
                {
                    hash = hashCombine(hash, item.second->hash());
                    hash = hashCombine(hash, std::hash<std::string>()(item.first));
                }

                return hash;
            }
        }
    }

public:
    bool operator!=(const Variant &other) const { return !(*this == other); }
    bool operator==(const Variant &other) const
    {
        /* must be the same type */
        if (_type != other._type)
            return false;

        /* and the same value */
        switch (_type)
        {
            case Type::TypeCode::Void    : return true;
            case Type::TypeCode::Int8    : return _s8  == other._s8 ;
            case Type::TypeCode::Int16   : return _s16 == other._s16;
            case Type::TypeCode::Int32   : return _s32 == other._s32;
            case Type::TypeCode::Int64   : return _s64 == other._s64;
            case Type::TypeCode::UInt8   : return _u8  == other._u8 ;
            case Type::TypeCode::UInt16  : return _u16 == other._u16;
            case Type::TypeCode::UInt32  : return _u32 == other._u32;
            case Type::TypeCode::UInt64  : return _u64 == other._u64;

            case Type::TypeCode::Float   : return _float  == other._float;
            case Type::TypeCode::Double  : return _double == other._double;
            case Type::TypeCode::String  : return _string == other._string;
            case Type::TypeCode::Boolean : return _bool   == other._bool;

            case Type::TypeCode::Map:
            {
                /* check the map size */
                if (_map.size() != other._map.size())
                    return false;

                /* compare each item */
                for (const auto &item : _map)
                {
                    /* locate in `other` map */
                    auto iter = other._map.find(item.first);

                    /* check for existence */
                    if (iter == other._map.end())
                        return false;

                    /* check for value */
                    if (*iter->second != *item.second)
                        return false;
                }

                /* all equals, then the two maps equals */
                return true;
            }

            case Type::TypeCode::Array:
            {
                /* check the array size */
                if (_array.size() != other._array.size())
                    return false;

                /* compare each item */
                for (auto x = _array.begin(), y = other._array.begin(); (x != _array.end()) && (y != other._array.end()); x++, y++)
                    if (**x != **y)
                        return false;

                /* all equals, then the two maps equals */
                return true;
            }

            case Type::TypeCode::Object:
            {
                /* check the object field count */
                if (_object.size() != other._object.size())
                    return false;

                /* compare each field */
                for (const auto &item : _object)
                {
                    /* locate in `other` object fields */
                    auto iter = other._object.find(item.first);

                    /* check for existence */
                    if (iter == other._object.end())
                        return false;

                    /* check for value */
                    if (*iter->second != *item.second)
                        return false;
                }

                /* all equals, then the two maps equals */
                return true;
            }
        }
    }

/** signed integers **/

private:
    struct TagInt8 {};
    struct TagInt16 {};
    struct TagInt32 {};
    struct TagInt64 {};

public:
    template <typename T>
    inline T get(std::enable_if_t<Internal::IsSignedIntegerLike<T, int8_t>::value, TagInt8> = TagInt8())
    {
        if (_type == Type::TypeCode::Int8)
            return static_cast<T>(_s8);
        else
            throw Exceptions::TypeError(toString() + " is not an `int8_t`");
    }

public:
    template <typename T>
    inline T get(std::enable_if_t<Internal::IsSignedIntegerLike<T, int16_t>::value, TagInt16> = TagInt16())
    {
        if (_type == Type::TypeCode::Int16)
            return static_cast<T>(_s16);
        else
            throw Exceptions::TypeError(toString() + " is not an `int16_t`");
    }

public:
    template <typename T>
    inline T get(std::enable_if_t<Internal::IsSignedIntegerLike<T, int32_t>::value, TagInt32> = TagInt32())
    {
        if (_type == Type::TypeCode::Int32)
            return static_cast<T>(_s32);
        else
            throw Exceptions::TypeError(toString() + " is not an `int32_t`");
    }

public:
    template <typename T>
    inline T get(std::enable_if_t<Internal::IsSignedIntegerLike<T, int64_t>::value, TagInt64> = TagInt64())
    {
        if (_type == Type::TypeCode::Int64)
            return static_cast<T>(_s64);
        else
            throw Exceptions::TypeError(toString() + " is not an `int64_t`");
    }

public:
    template <typename T>
    inline std::decay_t<T> get(std::enable_if_t<Internal::IsSignedIntegerLike<T, int8_t>::value, TagInt8> = TagInt8()) const
    {
        if (_type == Type::TypeCode::Int8)
            return static_cast<T>(_s8);
        else
            throw Exceptions::TypeError(toString() + " is not an `int8_t`");
    }

public:
    template <typename T>
    inline std::decay_t<T> get(std::enable_if_t<Internal::IsSignedIntegerLike<T, int16_t>::value, TagInt16> = TagInt16()) const
    {
        if (_type == Type::TypeCode::Int16)
            return static_cast<T>(_s16);
        else
            throw Exceptions::TypeError(toString() + " is not an `int16_t`");
    }

public:
    template <typename T>
    inline std::decay_t<T> get(std::enable_if_t<Internal::IsSignedIntegerLike<T, int32_t>::value, TagInt32> = TagInt32()) const
    {
        if (_type == Type::TypeCode::Int32)
            return static_cast<T>(_s32);
        else
            throw Exceptions::TypeError(toString() + " is not an `int32_t`");
    }

public:
    template <typename T>
    inline std::decay_t<T> get(std::enable_if_t<Internal::IsSignedIntegerLike<T, int64_t>::value, TagInt64> = TagInt64()) const
    {
        if (_type == Type::TypeCode::Int64)
            return static_cast<T>(_s64);
        else
            throw Exceptions::TypeError(toString() + " is not an `int64_t`");
    }

/** unsigned integers **/

private:
    struct TagUInt8 {};
    struct TagUInt16 {};
    struct TagUInt32 {};
    struct TagUInt64 {};

public:
    template <typename T>
    inline T get(std::enable_if_t<Internal::IsUnsignedIntegerLike<T, uint8_t>::value, TagUInt8> = TagUInt8())
    {
        if (_type == Type::TypeCode::UInt8)
            return static_cast<T>(_u8);
        else
            throw Exceptions::TypeError(toString() + " is not an `uint8_t`");
    }

public:
    template <typename T>
    inline T get(std::enable_if_t<Internal::IsUnsignedIntegerLike<T, uint16_t>::value, TagUInt16> = TagUInt16())
    {
        if (_type == Type::TypeCode::UInt16)
            return static_cast<T>(_u16);
        else
            throw Exceptions::TypeError(toString() + " is not an `uint16_t`");
    }

public:
    template <typename T>
    inline T get(std::enable_if_t<Internal::IsUnsignedIntegerLike<T, uint32_t>::value, TagUInt32> = TagUInt32())
    {
        if (_type == Type::TypeCode::UInt32)
            return static_cast<T>(_u32);
        else
            throw Exceptions::TypeError(toString() + " is not an `uint32_t`");
    }

public:
    template <typename T>
    inline T get(std::enable_if_t<Internal::IsUnsignedIntegerLike<T, uint64_t>::value, TagUInt64> = TagUInt64())
    {
        if (_type == Type::TypeCode::UInt64)
            return static_cast<T>(_u64);
        else
            throw Exceptions::TypeError(toString() + " is not an `uint64_t`");
    }

public:
    template <typename T>
    inline std::decay_t<T> get(std::enable_if_t<Internal::IsUnsignedIntegerLike<T, uint8_t>::value, TagUInt8> = TagUInt8()) const
    {
        if (_type == Type::TypeCode::UInt8)
            return static_cast<T>(_u8);
        else
            throw Exceptions::TypeError(toString() + " is not an `uint8_t`");
    }

public:
    template <typename T>
    inline std::decay_t<T> get(std::enable_if_t<Internal::IsUnsignedIntegerLike<T, uint16_t>::value, TagUInt16> = TagUInt16()) const
    {
        if (_type == Type::TypeCode::UInt16)
            return static_cast<T>(_u16);
        else
            throw Exceptions::TypeError(toString() + " is not an `uint16_t`");
    }

public:
    template <typename T>
    inline std::decay_t<T> get(std::enable_if_t<Internal::IsUnsignedIntegerLike<T, uint32_t>::value, TagUInt32> = TagUInt32()) const
    {
        if (_type == Type::TypeCode::UInt32)
            return static_cast<T>(_u32);
        else
            throw Exceptions::TypeError(toString() + " is not an `uint32_t`");
    }

public:
    template <typename T>
    inline std::decay_t<T> get(std::enable_if_t<Internal::IsUnsignedIntegerLike<T, uint64_t>::value, TagUInt64> = TagUInt64()) const
    {
        if (_type == Type::TypeCode::UInt64)
            return static_cast<T>(_u64);
        else
            throw Exceptions::TypeError(toString() + " is not an `uint64_t`");
    }

/** floating point numbers **/

private:
    struct TagFloat {};
    struct TagDouble {};

public:
    template <typename T>
    inline T get(std::enable_if_t<std::is_same<std::decay_t<T>, float>::value, TagFloat> = TagFloat())
    {
        if (_type == Type::TypeCode::Float)
            return static_cast<T>(_float);
        else
            throw Exceptions::TypeError(toString() + " is not a `float`");
    }

public:
    template <typename T>
    inline T get(std::enable_if_t<std::is_same<std::decay_t<T>, double>::value, TagDouble> = TagDouble())
    {
        if (_type == Type::TypeCode::Double)
            return static_cast<T>(_double);
        else
            throw Exceptions::TypeError(toString() + " is not a `double`");
    }

public:
    template <typename T>
    inline std::decay_t<T> get(std::enable_if_t<std::is_same<std::decay_t<T>, float>::value, TagFloat> = TagFloat()) const
    {
        if (_type == Type::TypeCode::Float)
            return static_cast<T>(_float);
        else
            throw Exceptions::TypeError(toString() + " is not a `float`");
    }

public:
    template <typename T>
    inline std::decay_t<T> get(std::enable_if_t<std::is_same<std::decay_t<T>, double>::value, TagDouble> = TagDouble()) const
    {
        if (_type == Type::TypeCode::Double)
            return static_cast<T>(_double);
        else
            throw Exceptions::TypeError(toString() + " is not a `double`");
    }

/** boolean **/

private:
    struct TagBoolean {};

public:
    template <typename T>
    inline T get(std::enable_if_t<std::is_same<std::decay_t<T>, bool>::value, TagBoolean> = TagBoolean())
    {
        if (_type == Type::TypeCode::Boolean)
            return static_cast<T>(_bool);
        else
            throw Exceptions::TypeError(toString() + " is not a `bool`");
    }

public:
    template <typename T>
    inline std::decay_t<T> get(std::enable_if_t<std::is_same<std::decay_t<T>, bool>::value, TagBoolean> = TagBoolean()) const
    {
        if (_type == Type::TypeCode::Boolean)
            return static_cast<T>(_bool);
        else
            throw Exceptions::TypeError(toString() + " is not a `bool`");
    }

/** STL string **/

private:
    struct TagString {};

public:
    template <typename T>
    inline T get(std::enable_if_t<std::is_same<std::decay_t<T>, std::string>::value, TagString> = TagString())
    {
        if (_type == Type::TypeCode::String)
            return _string;
        else
            throw Exceptions::TypeError(toString() + " is not a `std::string`");
    }

public:
    template <typename T>
    inline const std::string &get(std::enable_if_t<std::is_same<std::decay_t<T>, std::string>::value, TagString> = TagString()) const
    {
        if (_type == Type::TypeCode::String)
            return _string;
        else
            throw Exceptions::TypeError(toString() + " is not a `std::string`");
    }

/** Constant objects (maps, arrays and objects) **/

private:
    struct TagCMap {};
    struct TagCArray {};
    struct TagCObject {};

public:
    template <typename T>
    inline T get(std::enable_if_t<Internal::IsMap<T>::value, TagCMap> = TagCMap()) const
    {
        if (_type != Type::TypeCode::Map)
            throw Exceptions::TypeError(toString() + " is not a map");

        /* create result map */
        T map;

        /* fill each item */
        for (const auto &item : _map)
        {
            map.emplace(
                item.first.key->get<typename Internal::IsMap<T>::KeyType>(),
                item.second->get<typename Internal::IsMap<T>::ValueType>()
            );
        }

        /* move to prevent copy */
        return std::move(map);
    }

public:
    template <typename T>
    inline T get(std::enable_if_t<Internal::IsVector<T>::value, TagCArray> = TagCArray()) const
    {
        if (_type != Type::TypeCode::Array)
            throw Exceptions::TypeError(toString() + " is not an array");

        /* create result array */
        T array;

        /* fill each item */
        for (const auto &item : _array)
            array.push_back(item->get<typename Internal::IsVector<T>::ItemType>());

        /* move to prevent copy */
        return std::move(array);
    }

public:
    template <typename T>
    inline T get(std::enable_if_t<std::is_convertible<T *, Serializable *>::value, TagCObject> = TagCObject()) const
    {
        if (_type != Type::TypeCode::Object)
            throw Exceptions::TypeError(toString() + " is not an object");

        /* create result object */
        T object;

        /* deserialize from self */
        object.deserialize(*this);
        return std::move(object);
    }

/** Wrapped objects (arrays and objects) **/

private:
    struct TagWMap {};
    struct TagWType {};
    struct TagWArray {};
    struct TagWObject {};

public:
    template <typename T>
    inline T get(std::enable_if_t<Internal::IsTypeWrapper<T>::value, TagWType> = TagWType()) const
    {
        /* let the compiler decide which override should be used */
        return T(getWrapped<typename T::Type>());
    }

private:
    template <typename T>
    inline std::shared_ptr<T> getWrapped(std::enable_if_t<Internal::IsMap<T>::value, TagWMap> = TagWMap()) const
    {
        if (_type != Type::TypeCode::Map)
            throw Exceptions::TypeError(toString() + " is not a map");

        /* create result map */
        std::shared_ptr<T> map(new T);

        /* fill each item */
        for (const auto &item : _map)
        {
            map->emplace(
                item.first.key->get<typename Internal::IsMap<T>::KeyType>(),
                item.second->get<typename Internal::IsMap<T>::ValueType>()
            );
        }

        /* move to prevent copy */
        return std::move(map);
    }

private:
    template <typename T>
    inline std::shared_ptr<T> getWrapped(std::enable_if_t<Internal::IsVector<T>::value, TagWArray> = TagWArray()) const
    {
        if (_type != Type::TypeCode::Array)
            throw Exceptions::TypeError(toString() + " is not an array");

        /* create result array */
        std::shared_ptr<T> array(new T);

        /* fill each item */
        for (const auto &item : _array)
            array->push_back(item->get<typename Internal::IsVector<T>::ItemType>());

        /* move to prevent copy */
        return std::move(array);
    }

private:
    template <typename T>
    inline std::shared_ptr<T> getWrapped(std::enable_if_t<std::is_convertible<T *, Serializable *>::value, TagWObject> = TagWObject()) const
    {
        if (_type != Type::TypeCode::Object)
            throw Exceptions::TypeError(toString() + " is not an object");

        /* create result object */
        std::shared_ptr<T> object(new T);

        /* deserialize from self */
        object->deserialize(*this);
        return std::move(object);
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
    Variant &operator[](ssize_t index)
    {
        if (_type != Type::TypeCode::Array)
            throw Exceptions::TypeError(toString() + " is not an array");
        else if (index < 0 || static_cast<size_t>(index) >= _array.size())
            throw Exceptions::IndexError(index);
        else
            return *_array[index].get();
    }

public:
    const Variant &operator[](ssize_t index) const
    {
        if (_type != Type::TypeCode::Array)
            throw Exceptions::TypeError(toString() + " is not an array");
        else if (index < 0 || static_cast<size_t>(index) >= _array.size())
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
            _object.emplace(key, std::make_shared<Variant>(0));

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
    Map &internalMap(void) { return _map; }
    Array &internalArray(void) { return _array; }
    Object &internalObject(void) { return _object; }

public:
    const Map &internalMap(void) const { return _map; }
    const Array &internalArray(void) const { return _array; }
    const Object &internalObject(void) const { return _object; }

/** END **/

#pragma clang diagnostic push
#pragma ide diagnostic ignored "InfiniteRecursion"

private:
    template <typename ... Args>
    struct ArrayBuilder;

private:
    template <typename Arg, typename ... Args>
    struct ArrayBuilder<Arg, Args ...>
    {
        static void build(Array &array, Arg &&arg, Args && ... args)
        {
            array.emplace_back(std::make_shared<Variant>(std::forward<Arg>(arg)));
            ArrayBuilder<Args ...>::build(array, std::forward<Args>(args) ...);
        }
    };

#pragma clang diagnostic pop

public:
    template <typename ... Args>
    static Variant array(Args && ... args)
    {
        Variant result(Type::TypeCode::Array);
        std::vector<std::shared_ptr<Variant>> array;

        /* reserve spaces to prevent frequent malloc() */
        array.reserve(sizeof ... (Args));
        ArrayBuilder<Args ...>::build(array, std::forward<Args>(args) ...);

        /* replace internal array */
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
            result._object.emplace(pair.name, pair.value);

        return result;
    }

public:
    std::string toString(void) const
    {
        switch (_type)
        {
            /* void type */
            case Type::TypeCode::Void       : return "void";

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

            /* maps */
            case Type::TypeCode::Map:
            {
                std::string result;

                for (const auto &item : _map)
                {
                    if (!result.empty())
                        result += ", ";

                    result += item.first.key->toString();
                    result += ": ";
                    result += item.second->toString();
                }

                return "{" + result + "}";
            }

            /* arrays */
            case Type::TypeCode::Array:
            {
                std::string result;

                for (const auto &item : _array)
                {
                    if (!result.empty())
                        result += ", ";

                    result += item->toString();
                }

                return "[" + result + "]";
            }

            /* structs */
            case Type::TypeCode::Object:
            {
                std::string result;

                for (const auto &item : _object)
                {
                    if (!result.empty())
                        result += ", ";

                    result += ByteSeq::repr(item.first);
                    result += ": ";
                    result += item.second->toString();
                }

                return "<" + result + ">";
            }
        }
    }
};

/* variant key comparator */
inline bool VariantHashKey::operator==(const VariantHashKey &other) const
{
    /* forward to variant comparison function */
    return *key == *(other.key);
}

/* hash function for variant key */
inline size_t VariantHashKey::Hash::operator()(const VariantHashKey &key) const
{
    /* forward to variant hash function */
    return key.key->hash();
}

/* speciallizations must be placed out-side of the class */
template <>
struct Variant::ArrayBuilder<>
{
    static void build(Array &array)
    {
        /* last recursion, nothing left */
        /* thus nothing to do */
    }
};
}

#endif /* SIMPLERPC_VARIANT_H */
