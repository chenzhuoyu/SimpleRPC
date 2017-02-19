#include "Backend.h"

std::shared_ptr<SimpleRPC::Internal::Backend::BackendProxy> SimpleRPC::Internal::Backend::_defaultBackend;
std::unordered_map<std::string, std::shared_ptr<SimpleRPC::Internal::Backend::BackendProxy>> SimpleRPC::Internal::Backend::_backends;
