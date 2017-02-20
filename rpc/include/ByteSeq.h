/* Byte sequence for serialization / deserialization */

#ifndef SIMPLERPC_BYTESEQ_H
#define SIMPLERPC_BYTESEQ_H

#include <stack>
#include <string>
#include <algorithm>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

namespace SimpleRPC
{
namespace Internal
{
class ByteSeq
{
    char *_mem = nullptr;
    char *_readptr = nullptr;

private:
    size_t _length = 0;
    size_t _capacity = 0;

private:
    std::stack<size_t> _boundary;

private:
    static constexpr inline size_t alignUp(size_t value)
    {
        /* aligh up with 256 */
        return (value + 0xff) & ~0xff;
    }

public:
    ByteSeq() {}
   ~ByteSeq() { free(_mem); }

public:
    explicit ByteSeq(size_t initSize) { preserve(initSize); }
    explicit ByteSeq(const void *data, size_t size) { append(data, size); }

public:
    ByteSeq(const char *text) { append(text); }
    ByteSeq(const std::string &text) { append(text); }

private:
    ByteSeq(const ByteSeq &) = delete;
    ByteSeq &operator=(const ByteSeq &) = delete;

public:
    ByteSeq(ByteSeq &&other) { swap(other); }
    ByteSeq &operator=(ByteSeq &&other) { swap(other); return *this; }

public:
    void swap(ByteSeq &other);

public:
    char *data(void) const { return _readptr; }
    size_t length(void) const { return _length; }
    size_t capacity(void) const { return _capacity; }

public:
    void clear(void);
    void commit(size_t size) { _length += size; }

public:
    char *consume(size_t size);
    char *preserve(size_t size);

public:
    void append(const void *data, size_t size);

public:
    void append(const char *data)        { append(data, strlen(data)); }
    void append(const ByteSeq &data)     { append(data.data(), data.length()); }
    void append(const std::string &data) { append(data.data(), data.length()); }

public:
    size_t boundary(void) const
    {
        if (_boundary.empty())
            return _length;
        else
            return _boundary.top();
    }

public:
    void popBoundary(void)              { _boundary.pop(); }
    void pushBoundary(size_t boundary)  { _boundary.push(boundary); }

public:
    template <typename T>
    T nextLE(void)
    {
        char *p = consume(sizeof(T));
        return std::move(*reinterpret_cast<T *>(p));
    }

public:
    template <typename T>
    T nextBE(void)
    {
        char d[sizeof(T)];
        char *p = consume(sizeof(T));
        std::reverse_copy(p, p + sizeof(T), d);
        return std::move(*reinterpret_cast<T *>(d));
    }

public:
    template <typename T>
    void appendLE(const T &data)
    {
        const char *p = reinterpret_cast<const char *>(&data);
        append(p, sizeof(T));
    }

public:
    template <typename T>
    void appendBE(const T &data)
    {
        char d[sizeof(T)];
        const char *p = reinterpret_cast<const char *>(&data);
        std::reverse_copy(p, p + sizeof(T), d);
        append(d, sizeof(T));
    }

public:
    std::string repr(void) const { return repr(_readptr, _length); }
    std::string hexdump(void) const { return hexdump(_readptr, _length); }
    std::string toString(void) const { return std::string(_readptr, _length); }

public:
    static std::string repr(const std::string &data) { return repr(data.data(), data.size()); }
    static std::string hexdump(const std::string &data) { return hexdump(data.data(), data.size()); }

public:
    static std::string repr(const void *data, size_t size);
    static std::string hexdump(const void *data, size_t size);

};
}
}

#endif /* SIMPLERPC_BYTESEQ_H */
