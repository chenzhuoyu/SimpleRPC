/* Variant to byte-sequence serializer / deserializer backend */

#ifndef SIMPLERPC_BACKEND_H
#define SIMPLERPC_BACKEND_H

#include <string>
#include <memory>
#include <type_traits>
#include <unordered_map>

#include "Variant.h"
#include "Exceptions.h"

namespace SimpleRPC
{
namespace Internal
{
template <typename F>
struct NameCheck
{
    static constexpr bool value = false;
};

template <typename T>
struct NameCheck<std::string(T::*)() const>
{
    static constexpr bool value = true;
};

template <typename F>
struct ParserCheck
{
    static constexpr bool value = false;
};

template <typename T>
struct ParserCheck<Variant(T::*)(const std::string &) const>
{
    static constexpr bool value = true;
};

template <typename F>
struct AssemblerCheck
{
    static constexpr bool value = false;
};

template <typename T>
struct AssemblerCheck<std::string(T::*)(const Variant &) const>
{
    static constexpr bool value = true;
};

struct Backend final
{
    class BackendProxy
    {
        std::function<Variant(const std::string &)> _parser;
        std::function<std::string(const Variant &)> _assembler;

    public:
        explicit BackendProxy(
            std::function<Variant(const std::string &)> parser,
            std::function<std::string(const Variant &)> assembler) : _parser(parser), _assembler(assembler) {}

    public:
        Variant parse(const std::string &data) const { return _parser(data); }
        std::string assemble(const Variant &data) const { return _assembler(data); }

    };

private:
    static std::shared_ptr<BackendProxy> _defaultBackend;
    static std::unordered_map<std::string, std::shared_ptr<BackendProxy>> _backends;

public:
    template <typename T>
    static void addBackend(const std::string &name, std::shared_ptr<T> backend)
    {
        /* check for method existence in `T` */
        static_assert(
            Internal::ParserCheck<decltype(&T::parse)>::value &&
            Internal::AssemblerCheck<decltype(&T::assemble)>::value,
            "Backend must have `bool parse(const std::string &) const` and `std::string assemble(const Variant &) const` methods"
        );

        /* check for backend existence */
        if (_backends.find(name) != _backends.end())
            throw Exceptions::BackendDuplicatedError(name);

        /* build backend proxy and add to registry */
        _backends.insert({ name, std::make_shared<BackendProxy>(
            [=](const std::string &data) { return backend->parse(data);    },
            [=](const Variant     &data) { return backend->assemble(data); }
        ) });

        /* set as default backend if not specified */
        if (_defaultBackend == nullptr)
            _defaultBackend = _backends.at(name);
    }

public:
    static const std::shared_ptr<BackendProxy> &findBackend(const std::string &name)
    {
        if (_backends.find(name) != _backends.end())
            return _backends.at(name);
        else
            throw Exceptions::BackendNotFoundError(name);
    }

public:
    static const std::shared_ptr<BackendProxy> &defaultBackend(void)
    {
        if  (_defaultBackend != nullptr)
            return _defaultBackend;
        else
            throw Exceptions::RuntimeError("No default backend specified");
    }

public:
    static std::vector<std::string> backends(void)
    {
        std::vector<std::string> result;
        std::for_each(_backends.begin(), _backends.end(), [&](auto x){ result.push_back(x.first); });
        return result;
    }

public:
    static void setDefaultBackend(const std::string &name) { _defaultBackend = findBackend(name); }

public:
    static Variant parse(const std::string &data) { return defaultBackend()->parse(data); }
    static std::string assemble(const Variant &object) { return defaultBackend()->assemble(object); }

public:
    template <typename T>
    struct Register
    {
        static_assert(
            NameCheck<decltype(&T::name)>::value,
            "Backend must have `std::string name(void) const` method"
        );

        Register()
        {
            std::shared_ptr<T> backend = std::make_shared<T>();
            Backend::addBackend(backend->name(), backend);
        }
    };
};
}

/* export `Backend` class */
typedef Internal::Backend Backend;
}

#define defineBackend(type) static ::SimpleRPC::Internal::Backend::Register<type> __SimpleRPC_Backend_ ## type ## _DO_NOT_TOUCH_THIS_VARIABLE__;

#endif /* SIMPLERPC_BACKEND_H */
