#include <Poseidon/UI/Mods/ModsModule.hpp>
#include <Poseidon/Core/resincl.hpp>
#include <Poseidon/UI/GameModule.hpp>
#include <functional>

// DisplayMods + CreateDisplayMods live in OptionsUIApp.cpp alongside the other
// CreateDisplay* entry points — DisplayUI.hpp is not self-contained, so the
// display code has to compile in a translation unit that sets up the full UI
// include stack. This module just registers the menu item.
namespace Poseidon
{
void ModsModule::Register()
{
    GameModuleRegistry::Register({GameModuleId::Mods, "Mods", IDC_MAIN_MODS, CreateDisplayMods});
}
} // namespace Poseidon
