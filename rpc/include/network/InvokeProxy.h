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
        if (_site != nullptr)
            _site->cleanup(_id);

        /* then register a new ID if any site is present */
        if ((_site = site) != nullptr)
            _id = _site->startup(_class);
    }

public:
    template <typename R, typename ... Args>
    R invoke(const char *name, Args && ... args) const
    {
        /* check for call-site */
        if (_site == nullptr)
            throw std::runtime_error("Empty call site");

        /* define meta-method as `static` `thread_local` here, cause compiler will
         * generate a unique copy of this `invoke` method for every different template
         * arguments, thus the static variable `method` containing in those `invoke`
         * methods are all different and will be instaniated by corresponding template
         * types, the `thread_local` specifier is to prevent locks being used */
        static thread_local Internal::MetaMethod<R, Args ...> method;

        /* invoke actual method through call-site */
        return _site->invoke<R>(_id, name, method.signature, std::forward<Args>(args) ...);
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
