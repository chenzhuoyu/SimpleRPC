/* Local area call site */

#ifndef SIMPLERPC_LOCALCALLSITE_H
#define SIMPLERPC_LOCALCALLSITE_H

#include <string>
#include <memory>
#include <unordered_map>

#include "SimpleRPC.h"

namespace SimpleRPC
{
namespace Network
{
class LocalCallSite : public CallSite
{
    size_t _id = 0;
    std::unordered_map<size_t, std::unique_ptr<Serializable>> _objects;

public:
    virtual void cleanup(size_t id) noexcept override;
    virtual size_t startup(const std::string &name) override;

public:
    virtual Variant invoke(size_t id, const std::string &name, const std::string &signature, Variant &args) override;

};
}
}

#endif /* SIMPLERPC_LOCALCALLSITE_H */
