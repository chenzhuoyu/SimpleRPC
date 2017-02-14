#include "Exceptions.h"
#include "Serializable.h"

const SimpleRPC::Serializable::Meta &SimpleRPC::Serializable::meta(void) const
{
    std::lock_guard<std::mutex> _(_mutex);
    return _meta ? *_meta : *(_meta = &(Internal::Registry::findClass(_name)));
}
