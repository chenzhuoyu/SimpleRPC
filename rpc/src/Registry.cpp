#include "Variant.h"
#include "Registry.h"
#include "Inspector.h"
#include "Exceptions.h"

namespace SimpleRPC
{
namespace Internal
{
/* class factory registry */
static std::unordered_map<std::string, std::shared_ptr<Registry::Meta>> _registry;

void Registry::addClass(std::shared_ptr<Registry::Meta> &&meta)
{
    if (_registry.find(meta->name()) == _registry.end())
        _registry.emplace(meta->name(), std::move(meta));
}

const Registry::Meta &Registry::findClass(const std::string &name)
{
    if (_registry.find(name) != _registry.end())
        return *_registry.at(name);
    else
        throw Exceptions::ClassNotFoundError(name);
}

Variant Serializable::serialize(void) const
{
    /* create object type */
    Variant result(Type::TypeCode::Object);

    /* serialize each field */
    for (const auto &field : _meta.fields())
        result[field.first] = field.second->serialize(this);

    return result;
}

void Serializable::deserialize(const Variant &value)
{
    if (value.type() != Type::TypeCode::Object)
        throw Exceptions::TypeError(value.toString() + " is not an object");

    const auto &fields = _meta.fields();
    const auto &object = value.internalObject();

    for (const auto &field : object)
        if (fields.find(field.first) == fields.end())
            throw Exceptions::ReflectionError("No such field \"" + field.first + "\"");

    for (const auto &field : fields)
    {
        if (object.find(field.first) != object.end())
            field.second->deserialize(this, *object.at(field.first));
        else
            throw Exceptions::ReflectionError("Missing field \"" + field.first + "\"");
    }
}
}
}
