/* Class reflection registry */

#ifndef SIMPLERPC_REGISTRY_H
#define SIMPLERPC_REGISTRY_H

#include <string>
#include <unordered_map>

#include <cxxabi.h>
#include <stdlib.h>

namespace SimpleRPC
{
/****** Reflection registry ******/

namespace Internal
{
struct Field;
struct Method;
struct Serializable;

struct Registry
{
    class Meta
    {
        Meta(const Meta &) = delete;
        Meta &operator=(const Meta &) = delete;

    public:
        typedef std::unordered_map<std::string, std::shared_ptr<Field>> FieldMap;
        typedef std::unordered_map<std::string, std::shared_ptr<Method>> MethodMap;

    public:
        typedef Serializable *(* Constructor)(void);

    private:
        FieldMap _fields;
        MethodMap _methods;
        Constructor _constructor;

    private:
        std::string _name;

    public:
        explicit Meta() : _constructor(nullptr) {}
        explicit Meta(const std::string &name, FieldMap &&fields, MethodMap &&methods, Constructor &&constructor) :
            _name(name), _fields(std::move(fields)), _methods(std::move(methods)), _constructor(std::move(constructor)) {}

    public:
        Meta(Meta &&other)
        {
            std::swap(_fields, other._fields);
            std::swap(_methods, other._methods);
            std::swap(_constructor, other._constructor);
        }

    public:
        const std::string &name(void) const { return _name; }

    public:
        const FieldMap &fields(void) const { return _fields; }
        const MethodMap &methods(void) const { return _methods; }

    public:
        template <typename T> T *newInstance(void) const { return static_cast<T *>(_constructor()); }

    public:
        std::string readableName(void) const
        {
            int status;
            char *demangled = abi::__cxa_demangle(_name.c_str(), nullptr, nullptr, &status);
            std::string result = demangled ? std::string(demangled) : _name;

            free(demangled);
            return result;
        }
    };

private:
    Registry() = delete;
    ~Registry() = delete;

private:
    Registry(const Registry &) = delete;
    Registry &operator=(const Registry &) = delete;

public:
    static void addClass(std::shared_ptr<Meta> &&meta);
    static const Meta &findClass(const std::string &name);

};

/****** Serializable object container ******/

struct Serializable
{
    typedef Registry::Meta Meta;

protected:
    const Meta *_meta;

protected:
    virtual ~Serializable() {}
    explicit Serializable() {}

public:
    const Meta &meta(void) const { return *_meta; }

};
}
}

#endif //SIMPLERPC_REGISTRY_H
