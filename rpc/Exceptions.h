/* Exceptions used in this framework */

#ifndef SIMPLERPC_EXCEPTIONS_H
#define SIMPLERPC_EXCEPTIONS_H

#include <string>
#include <stdexcept>

namespace SimpleRPC
{
namespace Exceptions
{
/* Base exception class, you may alter it's implementation */
class Exception : public std::runtime_error
{
public:
    explicit Exception(const std::string &message) : runtime_error(message) {}

};

class ArrayIndexError : public Exception
{
public:
    explicit ArrayIndexError(ssize_t index) : Exception("Array index out of bound: %lld" + std::to_string(index)) {}

};
}
}

#endif /* SIMPLERPC_EXCEPTIONS_H */
