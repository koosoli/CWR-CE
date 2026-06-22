#include <Poseidon/UI/Campaigns/CampaignsModule.hpp>
#include <Poseidon/UI/GameModule.hpp>
#include <Poseidon/Core/resincl.hpp>
#include <functional>

namespace Poseidon
{
namespace Poseidon
{
void CreateDisplayCampaign(class ::ControlsContainer*);
}

extern void __cdecl CreateDisplayCampaign(ControlsContainer* parent);

void CampaignsModule::Register()
{
    GameModuleRegistry::Register({GameModuleId::Campaigns, "Campaigns", IDC_MAIN_GAME, CreateDisplayCampaign});
}

} // namespace Poseidon
