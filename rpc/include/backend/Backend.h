/* Variant to byte-sequence serializer / deserializer backend */

#ifndef SIMPLERPC_BACKEND_H
#define SIMPLERPC_BACKEND_H

#include <string>
#include <memory>
#include <vector>
#include <type_traits>
#include <unordered_map>

#include "ByteSeq.h"
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
struct ParserCheck<Variant(T::*)(ByteSeq &&) const>
{
    static constexpr bool value = true;
};

template <typename F>
struct AssemblerCheck
{
    static constexpr bool value = false;
};

template <typename T>
struct AssemblerCheck<ByteSeq(T::*)(Variant &&) const>
{
    static constexpr bool value = true;
};
}

struct Backend final
{
    class BackendProxy
    {
        std::function<Variant(ByteSeq &&)> _parser;
        std::function<ByteSeq(Variant &&)> _assembler;

    public:
        explicit BackendProxy(
            std::function<Variant(ByteSeq &&)> &&parser,
            std::function<ByteSeq(Variant &&)> &&assembler) :
                _parser(std::move(parser)), _assembler(std::move(assembler)) {}

    public:
        Variant parse(ByteSeq &&data) const { return _parser(std::move(data)); }
        ByteSeq assemble(Variant &&data) const { return _assembler(std::move(data)); }

    };

private:
    static std::shared_ptr<BackendProxy> &defaultBackendPtr(void)
    {
        static std::shared_ptr<BackendProxy> instance;
        return instance;
    }

private:
    static std::unordered_map<std::string, std::shared_ptr<BackendProxy>> &backendsMap(void)
    {
        static std::unordered_map<std::string, std::shared_ptr<BackendProxy>> instance;
        return instance;
    }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCSimplifyInspection"

public:
    template <typename T>
    static void addBackend(const std::string &name, std::shared_ptr<T> backend)
    {
        /* check for method existence in `T` */
        static_assert(
            Internal::ParserCheck<decltype(&T::parse)>::value &&
            Internal::AssemblerCheck<decltype(&T::assemble)>::value,
            "Backend must have `Variant parse(ByteSeq &&) const` and `ByteSeq assemble(Variant &&) const` methods"
        );

        /* check for backend existence */
        if (backendsMap().find(name) != backendsMap().end())
            throw Exceptions::BackendDuplicatedError(name);

        /* build backend proxy and add to registry */
        auto iter = backendsMap().emplace(name, std::make_shared<BackendProxy>(
            [=](ByteSeq &&data) { return backend->parse(std::move(data)); },
            [=](Variant &&data) { return backend->assemble(std::move(data)); }
        ));

        /* set as default backend if not specified */
        if (defaultBackendPtr() == nullptr)
            defaultBackendPtr() = iter.first->second;
    }

#pragma clang diagnostic pop

public:
    static const std::shared_ptr<BackendProxy> &findBackend(const std::string &name)
    {
        if (backendsMap().find(name) != backendsMap().end())
            return backendsMap().at(name);
        else
            throw Exceptions::BackendNotFoundError(name);
    }

public:
    static const std::shared_ptr<BackendProxy> &defaultBackend(void)
    {
        if  (defaultBackendPtr() != nullptr)
            return defaultBackendPtr();
        else
            throw Exceptions::RuntimeError("No default backend specified");
    }

public:
    static std::vector<std::string> backends(void)
    {
        std::vector<std::string> result;
        std::for_each(backendsMap().begin(), backendsMap().end(), [&](auto x){ result.push_back(x.first); });
        return result;
    }

public:
    static void setDefaultBackend(const std::string &name)
    {
        /* use function wrapper to get rid of the initialization order problem */
        defaultBackendPtr() = findBackend(name);
    }

public:
    static Variant parse(ByteSeq &&data) { return defaultBackend()->parse(std::move(data)); }
    static ByteSeq assemble(Variant &&object) { return defaultBackend()->assemble(std::move(object)); }

public:
    template <typename T>
    struct Register
    {
        static_assert(
            Internal::NameCheck<decltype(&T::name)>::value,
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

#define defineBackend(type) static ::SimpleRPC::Backend::Register<type> __SimpleRPC_Backend_ ## type ## _DO_NOT_TOUCH_THIS_VARIABLE__;

#endif /* SIMPLERPC_BACKEND_H */
