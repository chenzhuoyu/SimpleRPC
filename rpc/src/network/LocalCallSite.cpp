#include <memory>
#include "Registry.h"
#include "Inspector.h"
#include "network/LocalCallSite.h"

namespace SimpleRPC
{
namespace Network
{
Variant LocalCallSite::invoke(size_t id, const std::string &name, const std::string &signature, Variant &args)
{
    /* lookup object by ID */
    auto object = _objects.find(id);

    /* not found, it's an error */
    if (object == _objects.end())
        throw std::invalid_argument("ID " + std::to_string(id) + " is not registered");

    /* find method by signature */
    const auto &meta = object->second->meta();
    const auto &iter = meta.methods().find(name + signature);

    /* not found, it's an error */
    if (iter == meta.methods().end())
        throw std::invalid_argument("No such method \"" + name + "\" that has signature \"" + signature + "\"");

    /* otherwise, invoke it */
    return iter->second->invoke(object->second.get(), args);
}

void LocalCallSite::cleanup(size_t id) noexcept
{
    /* just erase from registry, `std::unique_ptr` will free the object */
    _objects.erase(id);
}

size_t LocalCallSite::startup(const std::string &name)
{
    size_t newId = __sync_fetch_and_add(&_id, 1);
    const auto &meta = Registry::findClass(name);

    /* instaniate object and register into object map */
    _objects.emplace(newId, std::unique_ptr<Serializable>(meta.newInstance<Serializable>()));
    return newId;
}
}
}
