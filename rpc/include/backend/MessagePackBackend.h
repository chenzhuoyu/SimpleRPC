/** MessagePack Serializer / Deserializer Backend
 *  for ``MessagePack Specification'' please refer to [https://github.com/msgpack/msgpack/blob/master/spec.md]
 **/

#ifndef SIMPLERPC_MESSAGEPACKBACKEND_H
#define SIMPLERPC_MESSAGEPACKBACKEND_H

#include <string>
#include "Backend.h"
#include "SimpleRPC.h"

namespace SimpleRPC
{
namespace Backends
{
struct MessagePackBackend
{
    std::string name(void) const { return "Backends.MessagePack"; }

private:
    Variant doParse(ByteSeq &seq) const;
    ByteSeq doAssemble(Variant &object) const;

public:
    Variant parse(ByteSeq &&data) const { return doParse(data); }
    ByteSeq assemble(Variant &&object) const { return doAssemble(object); }

};
}
}

#endif /* SIMPLERPC_MESSAGEPACKBACKEND_H */
