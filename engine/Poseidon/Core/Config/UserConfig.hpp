#pragma once

#include <Poseidon/Core/Profile/UserConfig.hpp>

// Convenience macro for accessing user config from game code.
// Usage: USER_CONFIG.IsEnabled(DTArmor)
// Expands to: GApp->GetConfig().GetUserConfig()
#include <Poseidon/Core/Application.hpp>


namespace Poseidon
{
class Application;
#define USER_CONFIG GApp->GetConfig().GetUserConfig()

// Game-specific helpers that load/save from the current player's profile path.
// Defined in UserConfig.cpp, used by ConfigFunctions.cpp and game state code.
void UserConfig_LoadDifficulties(UserConfig& uc);
void UserConfig_SaveDifficulties(const UserConfig& uc);
} // namespace Poseidon
