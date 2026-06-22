// Single source of truth for difficulty toggle names and defaults.

#include <Poseidon/Core/Profile/DifficultyTypes.hpp>

// clang-format off

namespace Poseidon
{
static DifficultyDesc s_diffDescs[DTN] = {
    DifficultyDesc("Armor",          0, true,  false, false),
    DifficultyDesc("FriendlyTag",    0, true,  false, false),
    DifficultyDesc("EnemyTag",       0, false, false, false),
    DifficultyDesc("HUD",            0, true,  false, false),
    DifficultyDesc("AutoSpot",       0, true,  false, false),
    DifficultyDesc("Map",            0, true,  false, false),
    DifficultyDesc("WeaponCursor",   0, true,  true,  true),
    DifficultyDesc("AutoGuideAT",    0, true,  false, false),
    DifficultyDesc("ClockIndicator", 0, true,  true,  true),
    DifficultyDesc("3rdPersonView",  0, true,  true,  true),
    DifficultyDesc("Tracers",        0, true,  true,  true),
    DifficultyDesc("UltraAI",        0, false, false, true),
};
// clang-format on

DifficultyDesc* GetDifficultyDescs()
{
    return s_diffDescs;
}
} // namespace Poseidon
