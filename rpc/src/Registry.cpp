#include "Variant.h"
#include "Registry.h"
#include "Inspector.h"
#include "Exceptions.h"

/* class factory registry */
static std::unordered_map<std::string, std::shared_ptr<SimpleRPC::Internal::Registry::Meta>> _registry;

void SimpleRPC::Internal::Registry::addClass(std::shared_ptr<SimpleRPC::Internal::Registry::Meta> &&meta)
{
    if (_registry.find(meta->name()) == _registry.end())
        _registry.insert({ meta->name(), meta });
    else
        throw SimpleRPC::Exceptions::ClassDuplicatedError(meta->name());
}

const SimpleRPC::Internal::Registry::Meta &SimpleRPC::Internal::Registry::findClass(const std::string &name)
{
    if (_registry.find(name) != _registry.end())
        return *_registry.at(name);
    else
        throw SimpleRPC::Exceptions::ClassNotFoundError(name);
}

SimpleRPC::Internal::Variant SimpleRPC::Internal::Serializable::serialize(void) const
{
    /* create object type */
    Variant result(Type::TypeCode::Struct);

    /* serialize each field */
    for (const auto &field : _meta->fields())
        result[field.first] = field.second->serialize(this);

    return result;
}

void SimpleRPC::Internal::Serializable::deserialize(const SimpleRPC::Internal::Variant &value)
{
    if (value.type() != Type::TypeCode::Struct)
        throw Exceptions::TypeError(value.toString() + " is not an object");

    auto keys = value.keys();
    auto fields = _meta->fields();

    for (const auto &field : keys)
        if (fields.find(field) == fields.end())
            throw Exceptions::ReflectionError("No such field \"" + field + "\"");

    for (const auto &field : fields)
    {
        if (std::find(keys.begin(), keys.end(), field.first) != keys.end())
            field.second->deserialize(this, value[field.first]);
        else
            throw Exceptions::ReflectionError("Missing field \"" + field.first + "\"");
    }
}
