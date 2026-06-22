#include <Poseidon/UI/Multiplayer/MultiplayerModule.hpp>
#include <Poseidon/UI/GameModule.hpp>
#include <Poseidon/Core/resincl.hpp>
#include <functional>

namespace Poseidon
{

void MultiplayerModule::Register()
{
    GameModuleRegistry::Register(
        {GameModuleId::Multiplayer, "Multiplayer", IDC_MAIN_MULTIPLAYER, CreateDisplayMultiplayer});
}

} // namespace Poseidon
