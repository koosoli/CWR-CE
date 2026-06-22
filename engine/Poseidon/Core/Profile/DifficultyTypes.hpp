// Difficulty system types shared between game and launcher.

#pragma once


/// Difficulty option types

namespace Poseidon
{
enum DifficultyType
{
    DTArmor,
    DTFriendlyTag,
    DTEnemyTag,
    DTHUD,
    DTAutoSpot,
    DTMap,
    DTWeaponCursor,
    DTAutoGuideAT,
    DTClockIndicator,
    DT3rdPersonView,
    DTTracers,
    DTUltraAI,
    DTN // terminator / count
};

/// Describes a single difficulty toggle
struct DifficultyDesc
{
    const char* name;      // Key name for cfg ("Armor", "FriendlyTag", etc.)
    int stringId;          // IDS for localized string (patched at runtime by game)
    bool defaultCadet;     // Default value for cadet mode
    bool defaultVeteran;   // Default value for veteran mode
    bool enabledInVeteran; // Whether this setting applies in veteran mode

    DifficultyDesc(const char* n, int sid, bool cadet, bool veteran, bool enabled)
        : name(n), stringId(sid), defaultCadet(cadet), defaultVeteran(veteran), enabledInVeteran(enabled)
    {
    }
};

/// Get the canonical difficulty descriptors array (DTN entries).
/// Returns static data with stringId=0; game patches string IDs at runtime.
DifficultyDesc* GetDifficultyDescs();
} // namespace Poseidon
