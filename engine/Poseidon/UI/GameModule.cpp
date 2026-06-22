#include <Poseidon/UI/GameModule.hpp>
#include <algorithm>
#include <utility>

namespace Poseidon
{
std::vector<GameModuleInfo>& GameModuleRegistry::Modules()
{
    static std::vector<GameModuleInfo> modules;
    return modules;
}

void GameModuleRegistry::Register(GameModuleInfo info)
{
    auto& mods = Modules();
    // Replace if already registered (allows re-registration in tests)
    for (auto& m : mods)
    {
        if (m.id == info.id)
        {
            m = std::move(info);
            return;
        }
    }
    mods.push_back(std::move(info));
}

bool GameModuleRegistry::IsRegistered(GameModuleId id)
{
    return Find(id) != nullptr;
}

const GameModuleInfo* GameModuleRegistry::Find(GameModuleId id)
{
    auto& mods = Modules();
    auto it = std::find_if(mods.begin(), mods.end(), [id](const GameModuleInfo& m) { return m.id == id; });
    return it != mods.end() ? &(*it) : nullptr;
}

const GameModuleInfo* GameModuleRegistry::FindByIDC(int idc)
{
    auto& mods = Modules();
    auto it = std::find_if(mods.begin(), mods.end(), [idc](const GameModuleInfo& m) { return m.menuButtonIDC == idc; });
    return it != mods.end() ? &(*it) : nullptr;
}

const std::vector<GameModuleInfo>& GameModuleRegistry::All()
{
    return Modules();
}

void GameModuleRegistry::Clear()
{
    Modules().clear();
}

} // namespace Poseidon
