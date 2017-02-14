#include "Inspector.h"

/* class factory registry */
static std::unordered_map<std::string, std::shared_ptr<SimpleRPC::Internal::Registry::Meta>> _registry;

void SimpleRPC::Internal::Registry::addClass(const std::string &name, std::shared_ptr<SimpleRPC::Internal::Registry::Meta> &&meta)
{
    if (_registry.find(name) == _registry.end())
        _registry.insert({ name, meta });
}

const SimpleRPC::Internal::Registry::Meta &SimpleRPC::Internal::Registry::findClass(const std::string &name)
{
    if (_registry.find(name) != _registry.end())
        return *_registry.at(name);
    else
        throw SimpleRPC::Exceptions::ClassNotFoundError(name);
}
