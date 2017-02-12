#include "TypeTraits.h"

/* class factory registry */
static std::unordered_map<std::string, SimpleRPC::Struct::Meta> _registry;

const SimpleRPC::Struct::Meta &SimpleRPC::Struct::meta(void) const
{
    std::lock_guard<std::mutex> _(_mutex);
    return _meta ? *_meta : *(_meta = &(_registry.at(_name)));
}

void SimpleRPC::Registry::addClass(const std::string &name, const SimpleRPC::Struct::Meta &meta)
{
    if (_registry.find(name) == _registry.end())
        _registry.insert({ name, meta });
}

SimpleRPC::Struct::Meta &SimpleRPC::Registry::findClass(const std::string &name)
{
    if (_registry.find(name) != _registry.end())
        return _registry.at(name);
    else
        throw SimpleRPC::Exceptions::ClassNotFoundError(name);
}
