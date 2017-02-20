#include "Backend.h"

namespace SimpleRPC
{
namespace Internal
{
std::shared_ptr<Backend::BackendProxy> Backend::_defaultBackend;
std::unordered_map<std::string, std::shared_ptr<Backend::BackendProxy>> Backend::_backends;
}
}
