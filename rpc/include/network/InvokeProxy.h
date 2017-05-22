/* Proxy class for any method invocations */

#ifndef SIMPLERPC_INVOKEPROXY_H
#define SIMPLERPC_INVOKEPROXY_H

#include <string>
#include <stdexcept>

#include "CallSite.h"
#include "TypeInfo.h"

namespace SimpleRPC
{
namespace Network
{
class InvokeProxy
{
    size_t _id;
    CallSite *_site;
    std::string _class;

public:
    virtual ~InvokeProxy() { setSite(nullptr); }
    explicit InvokeProxy(CallSite *site, const std::string &name) : _id(0), _site(nullptr), _class(name) { setSite(site); }

public:
    size_t id(void) const { return _id; }
    CallSite *site(void) const { return _site; }

public:
    void setSite(CallSite *site)
    {
        /* cleanup previous registered ID if any */
        if (_site)
            _site->cleanup(_id);

        /* then register a new ID if any site is present */
        if ((_site = site))
            _id = _site->startup(_class);
    }

public:
    template <typename R, typename ... Args>
    R invoke(const char *name, Args && ... args) const
    {
        if (_site == nullptr)
            throw std::runtime_error("Empty call site");
        else
            return _site->invoke(_id, Internal::MetaMethod<R, Args ...>(name), std::forward<Args>(args) ...);
    }
};

template <typename T>
struct InvokeProxyAdapter : public InvokeProxy
{
    explicit InvokeProxyAdapter(CallSite *site) :
        InvokeProxy(site, Internal::TypeItem<T>::type().toSignature()) {}
};
}
}

#endif /* SIMPLERPC_INVOKEPROXY_H */
