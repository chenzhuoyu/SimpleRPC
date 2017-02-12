/* Serializable registry */

#ifndef SIMPLERPC_REGISTRY_H
#define SIMPLERPC_REGISTRY_H

#include <string>

namespace SimpleRPC
{
namespace Registry
{
class Struct;
typedef Struct *(* FactoryFunction)(void);

void addClass(const std::string &name, FactoryFunction factory);
Struct *newInstance(const std::string &name);
}
}

#endif /* SIMPLERPC_REGISTRY_H */
