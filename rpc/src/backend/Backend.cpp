#include "backend/Backend.h"

namespace SimpleRPC
{
std::shared_ptr<Backend::BackendProxy> Backend::_defaultBackend;
std::unordered_map<std::string, std::shared_ptr<Backend::BackendProxy>> Backend::_backends;
}
