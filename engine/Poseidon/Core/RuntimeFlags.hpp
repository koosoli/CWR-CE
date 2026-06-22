#pragma once

// Engine-internal runtime flags. Set by CLI arguments or engine internals at
// runtime; never serialized to config files, not shared with launcher.

namespace Poseidon
{
struct RuntimeFlags
{
    bool checkInitAndExit = false;
    bool noLandscape = false;
    bool noTerrainCache = false;
    int showFps = 0; // 0=off, 1=basic, 2/3=detailed
    bool gUseFileBanks = true;
    bool gEnableCaching = true;
    bool gReplaceProxies = true;
    bool gMergeTextures = false;
    bool landEditor = false;
    bool hideCursor = false;
    bool noRedrawWindow = false;
    bool blood = true;
    bool hostMultiplayer = false; // Game acts as MP host (--host)
    bool dedicatedServer = false; // Headless server mode (--server / PoseidonServer)
#if _ENABLE_CHEATS
    bool super = false;
#endif
};
} // namespace Poseidon
