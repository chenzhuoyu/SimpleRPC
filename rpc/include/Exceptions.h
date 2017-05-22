/* Exceptions used in this framework */

#ifndef SIMPLERPC_EXCEPTIONS_H
#define SIMPLERPC_EXCEPTIONS_H

#include <string>
#include <typeinfo>
#include <stdexcept>

namespace SimpleRPC
{
namespace Exceptions
{
/* Base exception class, you may alter it's implementation */
class Exception : public std::runtime_error
{
public:
    explicit Exception(const std::string &message) : std::runtime_error(message) {}

};

class TypeError : public Exception
{
public:
    explicit TypeError(const std::string &message) : Exception(message) {}

};

class NameError : public Exception
{
public:
    explicit NameError(const std::string &name) : Exception("No such name: " + name) {}

};

class IndexError : public Exception
{
public:
    explicit IndexError(ssize_t index) : Exception("Array index out of bound: " + std::to_string(index)) {}

};

class ValueError : public Exception
{
public:
    explicit ValueError(const std::string &message) : Exception(message) {}

};

class RuntimeError : public Exception
{
public:
    explicit RuntimeError(const std::string &message) : Exception(message) {}

};

class ArgumentError : public Exception
{
public:
    explicit ArgumentError(size_t formal, size_t actual) :
        Exception("Method declared as " + std::to_string(formal) + " parameter(s) but got " + std::to_string(actual)) {}

};

class ReflectionError : public Exception
{
public:
    explicit ReflectionError(const std::string &message) : Exception(message) {}

};

class SerializerError : public Exception
{
public:
    explicit SerializerError(const std::string &message) : Exception(message) {}

};

class DeserializerError : public Exception
{
public:
    explicit DeserializerError(const std::string &message) : Exception(message) {}

};

class BufferOverflowError : public Exception
{
public:
    explicit BufferOverflowError(size_t size) : Exception("Buffer overflow : " + std::to_string(size)) {}

};

class ClassNotFoundError : public Exception
{
public:
    explicit ClassNotFoundError(const std::string &name) : Exception("Class \"" + name + "\" not found in registry") {}

};

class ClassDuplicatedError : public Exception
{
public:
    explicit ClassDuplicatedError(const std::string &name) : Exception("Class \"" + name + "\" duplicated in registry") {}

};

class BackendNotFoundError : public Exception
{
public:
    explicit BackendNotFoundError(const std::string &name) : Exception("Backend \"" + name + "\" not found in registry") {}

};

class BackendDuplicatedError : public Exception
{
public:
    explicit BackendDuplicatedError(const std::string &name) : Exception("Backend \"" + name + "\" duplicated in registry") {}

};
}
}

#endif /* SIMPLERPC_EXCEPTIONS_H */
