#include "TypeTraits.h"

/* class factory registry */
static std::unordered_map<std::string, SimpleRPC::Registry::Meta> _registry;

void SimpleRPC::Registry::addClass(const std::string &name, const SimpleRPC::Registry::Meta &meta)
{
    if (_registry.find(name) == _registry.end())
        _registry.insert({ name, meta });
}

SimpleRPC::Registry::Meta &SimpleRPC::Registry::findClass(const std::string &name)
{
    if (_registry.find(name) != _registry.end())
        return _registry.at(name);
    else
        throw SimpleRPC::Exceptions::ClassNotFoundError(name);
}
