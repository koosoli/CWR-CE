#include <Poseidon/UI/Missions/MissionsModule.hpp>
#include <Poseidon/UI/GameModule.hpp>
#include <Poseidon/Core/resincl.hpp>
#include <functional>

namespace Poseidon
{

void MissionsModule::Register()
{
    GameModuleRegistry::Register(
        {GameModuleId::Missions, "Single Missions", IDC_MAIN_SINGLE, CreateDisplaySingleMission});
}

} // namespace Poseidon
