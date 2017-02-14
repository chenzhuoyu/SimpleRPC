/* Exceptions used in this framework */

#ifndef SIMPLERPC_EXCEPTIONS_H
#define SIMPLERPC_EXCEPTIONS_H

#include <string>
#include <typeinfo>
#include <stdexcept>

#include <cxxabi.h>
#include <stdlib.h>

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

class ArgumentError : public Exception
{
public:
    explicit ArgumentError(size_t formal, size_t actual) :
        Exception("Method declared as " + std::to_string(formal) + " parameter(s) but got " + std::to_string(actual)) {}

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

class UnknownComplexTypeError : public Exception
{
    std::string _typeName;

private:
    static inline std::string demangle(const char *name)
    {
        int status;
        char *demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
        std::string result = demangled ?: name;

        free(demangled);
        return result;
    }

public:
    explicit UnknownComplexTypeError(const std::type_info &type) : _typeName(demangle(type.name())), Exception("Unknown complex type \"" + std::string(demangle(type.name())) + "\"") {}

public:
    const std::string &typeName(void) const { return _typeName; }

};
}
}

#endif /* SIMPLERPC_EXCEPTIONS_H */
