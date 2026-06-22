#include "GameDemoApplication.hpp"
#include <Poseidon/UI/Missions/MissionsModule.hpp>
#include <Poseidon/UI/Editor/EditorModule.hpp>

void GameDemoApplication::RegisterGameModules()
{
    // Demo ships Single Missions only; unregistered modules stay disabled in the main menu.
    Poseidon::MissionsModule::Register();
    // Poseidon::CampaignsModule::Register();
    // Poseidon::MultiplayerModule::Register();
    // Poseidon::EditorModule::Register();
    // ModsModule::Register();
}
