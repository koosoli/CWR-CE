#pragma once

#include <functional>
#include <vector>

class ControlsContainer;

namespace Poseidon
{
// Identifies a game mode module
enum class GameModuleId
{
    Missions,     ///< Single missions (IDC_MAIN_SINGLE)
    Campaigns,    ///< Campaign game (IDC_MAIN_GAME)
    Multiplayer,  ///< Multiplayer (IDC_MAIN_MULTIPLAYER)
    Editor,       ///< Mission editor (IDC_MAIN_EDITOR)
    Mods,         ///< Mod browser (IDC_MAIN_MODS)
};

// Describes a registered game module
struct GameModuleInfo
{
    GameModuleId id;
    const char* name;                                     ///< Display name ("Single Missions", etc.)
    int menuButtonIDC;                                    ///< Main menu button IDC to enable
    std::function<void(ControlsContainer*)> menuAction;   ///< Creates the entry Display on click
};

// Static registry of active game modules.
// Populated by the application at startup before engine init.
class GameModuleRegistry
{
public:
    static void Register(GameModuleInfo info);
    static bool IsRegistered(GameModuleId id);
    static const GameModuleInfo* Find(GameModuleId id);
    static const GameModuleInfo* FindByIDC(int idc);
    static const std::vector<GameModuleInfo>& All();
    static void Clear();

private:
    static std::vector<GameModuleInfo>& Modules();
};

void __cdecl CreateDisplaySingleMission(ControlsContainer* parent);
void __cdecl CreateDisplayMultiplayer(ControlsContainer* parent);
void __cdecl CreateDisplayEditor(ControlsContainer* parent);
void __cdecl CreateDisplayMods(ControlsContainer* parent);

} // namespace Poseidon
