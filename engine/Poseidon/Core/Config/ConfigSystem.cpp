#include <Poseidon/Core/Config/ConfigSystem.hpp>

namespace Poseidon
{
ConfigSystem& ConfigSystem::Instance()
{
    static ConfigSystem instance;
    return instance;
}

void ConfigSystem::Shutdown()
{
    _configAvailable = false;
    _resourceAvailable = false;
    _remasterAvailable = false;
}
} // namespace Poseidon
