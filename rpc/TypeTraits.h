/* Basic type-traits */

#ifndef SIMPLERPC_TYPETRAITS_H
#define SIMPLERPC_TYPETRAITS_H

#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>

#include <cxxabi.h>
#include <stdint.h>

#include "Registry.h"
#include "Exceptions.h"

namespace SimpleRPC
{
class Type
{
    std::string _className;
    std::shared_ptr<Type> _itemType;

public:
    enum TypeCode
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
    explicit Type(TypeCode typeCode) : _typeCode(typeCode) {}
    explicit Type(TypeCode typeCode, const std::string &className) : _typeCode(typeCode), _className(className) {}

public:
    /* array sub-item type */
    explicit Type(TypeCode typeCode, const Type &itemType) : _typeCode(typeCode), _itemType(new Type(itemType._typeCode))
    {
        _itemType->_itemType = itemType._itemType;
        _itemType->_className = itemType._className;
    }

public:
    Type itemType(void) const { return *_itemType; }
    TypeCode typeCode(void) const { return _typeCode; }
    std::string className(void) const { return _className; }

};

class Field
{
    Type _type;
    bool _optional;

private:
    void *_data;
    std::string _name;

protected:
    template <typename T>
    friend struct Array;
    friend struct Struct;

public:
    explicit Field(Type type, void *data, const std::string &name, bool optional) :
        _type(type), _name(name), _data(data), _optional(optional) {}

public:
    Type type(void) const { return _type; }
    std::string name(void) const { return _name; }

public:
    bool isOptional(void) const { return _optional; }

public:
    template <typename T> T &data(void) const { return *(T *)_data; }

};

template <typename T>
class Array;
class Struct;

template <typename T>
inline Type resolveType(const Array<T> &)
{
    T dummy;
    return Type(Type::Array, resolveType(dummy));
}

template <> inline Type resolveType<int8_t >(const int8_t  &) { return Type(Type::Int8 ); }
template <> inline Type resolveType<int16_t>(const int16_t &) { return Type(Type::Int16); }
template <> inline Type resolveType<int32_t>(const int32_t &) { return Type(Type::Int32); }
template <> inline Type resolveType<int64_t>(const int64_t &) { return Type(Type::Int64); }

template <> inline Type resolveType<uint8_t >(const uint8_t  &) { return Type(Type::UInt8 ); }
template <> inline Type resolveType<uint16_t>(const uint16_t &) { return Type(Type::UInt16); }
template <> inline Type resolveType<uint32_t>(const uint32_t &) { return Type(Type::UInt32); }
template <> inline Type resolveType<uint64_t>(const uint64_t &) { return Type(Type::UInt64); }

template <> inline Type resolveType<float >(const float  &) { return Type(Type::Float ); }
template <> inline Type resolveType<double>(const double &) { return Type(Type::Double); }

template <> inline Type resolveType<std::string>(const std::string &) { return Type(Type::String); }


template <typename T>
class Array
{
    T **_values = nullptr;
    ssize_t _length = 0;

private:
    friend class Parser;
    T &valueAt(ssize_t index) { return *_values[index]; }

private:
    void create(ssize_t size)
    {
        _length = size;
        _values = new T* [_length];

        for (int i = 0; i < _length; i++)
            _values[i] = new T;
    }

private:
    void create(ssize_t size, const std::string &className)
    {
        _length = size;
        _values = new T* [_length];

        for (int i = 0; i < _length; i++)
            _values[i] = Registry::newInstance(className);
    }

private:
    Field makeField(ssize_t index, const Type &itemType, const Field &parent)
    {
        return Field(itemType, (void *)_values[index], "_" + std::to_string(index), false);
    }

public:
    class iterator
    {
        T **_data;
        ssize_t _index;
        ssize_t _length;

    public:
        iterator(const iterator &o) : iterator(o._data, o._index, o._length) {}
        iterator(T **data, ssize_t index, ssize_t length) : _data(data), _index(index), _length(length) {}

        bool operator!=(const iterator &other) { return _data != other._data || _index != other._index; }
        bool operator==(const iterator &other) { return _data == other._data && _index == other._index; }

        const T &operator*(void)  { if (_index < _length) return *_data[_index]; else throw Exceptions::ArrayIndexError(_index); }
        const T *operator->(void) { if (_index < _length) return  _data[_index]; else throw Exceptions::ArrayIndexError(_index); }

        iterator operator++(int)  { return iterator(_data, _index++, _length); }
        iterator operator++(void) { return iterator(_data, ++_index, _length); }

    };

public:
    virtual ~Array()
    {
        for (size_t i = 0; i < _length; i++)
            delete _values[i];

        delete _values;
    }

public:
    ssize_t size(void)   const { return _length; }
    ssize_t length(void) const { return _length; }

public:
    iterator end  (void) const { return iterator(_values, _length, _length); }
    iterator begin(void) const { return iterator(_values,       0, _length); }

public:
    const T &operator[](ssize_t index) const
    {
        /* array not initialized */
        if (!_values)
            throw Exceptions::ArrayIndexError(index);

        /* negative index support */
        ssize_t pos = index >= 0 ? index : index + _length;

        /* but not too much "negative" */
        if (pos >= 0 && pos < _length)
            return *_values[pos];
        else
            throw Exceptions::ArrayIndexError(index);
    }
};
}

#endif /* SIMPLERPC_TYPETRAITS_H */
