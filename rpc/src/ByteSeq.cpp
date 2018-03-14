#include <utility>
#include "ByteSeq.h"
#include "Exceptions.h"

namespace SimpleRPC
{
void ByteSeq::swap(ByteSeq &other)
{
    std::swap(_mem, other._mem);
    std::swap(_length, other._length);
    std::swap(_readptr, other._readptr);
    std::swap(_capacity, other._capacity);
}

void ByteSeq::clear(void)
{
    _length = 0;
    _readptr = _mem;
}

char *ByteSeq::consume(size_t size)
{
    /* no data left */
    if (!_length)
        throw Exceptions::BufferOverflowError(_length);

    /* current position is the start position of read */
    char *result = _readptr;

    /* avoid buffer overflow */
    if (size > _length)
        throw Exceptions::BufferOverflowError(_length);

    /* simply move read pointer */
    _length -= size;
    _readptr += size;
    return result;
}

char *ByteSeq::preserve(size_t size)
{
    /* flags for resize */
    bool resize = false;

    /* find smallest size we need */
    while (size > _capacity - _length)
    {
        resize = true;
        _capacity = (_capacity == 0) ? 32 : (_capacity * 2);
    }

    /* doesn't consumed anything */
    if (_mem == _readptr)
    {
        /* if neither resize needed, do nothing */
        if (resize)
        {
            /* simply realloc */
            _mem = reinterpret_cast<char *>(realloc(_mem, _capacity));
            _readptr = _mem;
        }
        else if (_readptr == nullptr)
        {
            /* if `Buffer` is at initial state, and required no bytes (size == 0) */
            if (_mem == nullptr)
            {
                /* simply allocate a new buffer */
                _mem = reinterpret_cast<char *>(malloc(_capacity));
                _readptr = _mem;
            }
            else
            {
                /* neither, something went horribly wrong */
                abort();
            }
        }
    }
    else
    {
        /* consumed some bytes */
        if (!resize)
        {
            /* check for remaining spaces after data chunk */
            if (_capacity - _length - (_readptr - _mem) < size)
            {
                /* resize not needed, but not enough space left, simply memmove */
                memmove(_mem, _readptr, _length);
                _readptr = _mem;
            }
        }
        else
        {
            /* allocate another new space */
            void *newSpace = malloc(_capacity);

            /* discard data that already consumed */
            memcpy(newSpace, _readptr, _length);
            free(_mem);

            /* reset readptr and memory pointer */
            _mem = reinterpret_cast<char *>(newSpace);
            _readptr = reinterpret_cast<char *>(newSpace);
        }
    }

    /* return the available address */
    return _readptr + _length;
}

void ByteSeq::append(const void *data, size_t size)
{
    /* copy to end of buffer, then commit */
    memcpy(preserve(size), data, size);
    commit(size);
}

std::string ByteSeq::repr(const void *data, size_t size)
{
    if (!data)
        return "(nullptr)";

    auto p = reinterpret_cast<const uint8_t *>(data);
    std::string result = "\"";

    while (size--)
    {
        uint8_t ch = *p++;

        if (ch == '\"')
        {
            result += '\\';
            result += ch;
            continue;
        }

        switch (ch)
        {
            case '\\':
            {
                result += '\\';
                result += '\\';
                break;
            }

            case '\t':
            {
                result += '\\';
                result += 't';
                break;
            }

            case '\n':
            {
                result += '\\';
                result += 'n';
                break;
            }

            case '\r':
            {
                result += '\\';
                result += 'r';
                break;
            }

            default:
            {
                if (ch >= ' ' && ch < 0x7f)
                {
                    result += static_cast<char>(ch);
                    break;
                }

                result += '\\';
                result += 'x';
                result += "0123456789abcdef"[(ch & 0xf0) >> 4];
                result += "0123456789abcdef"[(ch & 0x0f) >> 0];
                break;
            }
        }
    }

    result += '\"';
    return result;
}

std::string ByteSeq::hexdump(const void *data, size_t size)
{
    if (!data)
        return "(nullptr)";

    auto p = reinterpret_cast<const uint8_t *>(data);
    char buffer[26] = {0};
    std::string result;

    for (size_t i = 0, r = 0; r < (size / 16 + (size % 16 != 0)); r++, i += 16)
    {
        snprintf(buffer, sizeof(buffer), "%08zu | ", i);
        result += buffer;

        for (size_t c = i; c < i + 16; c++)
        {
            if (c >= size)
            {
                result += "   ";
                continue;
            }

            if (c == i + 8)
                result += " ";

            snprintf(buffer, sizeof(buffer), "%02x ", p[c]);
            result += buffer;
        }

        for (size_t c = i; c < i + 16; c++)
        {
            if (c == i)
                result += " | ";

            if (c >= size)
                result += " ";
            else if (p[c] < ' ' || p[c] >= 0x7f)
                result += ".";
            else
                result += static_cast<char>(p[c]);
        }

        result += "\n";
    }

    return result;
}
}
