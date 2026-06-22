#pragma once

namespace Poseidon
{

inline constexpr const char* MasterServerFieldGroupId = "groupid";
inline constexpr const char* MasterServerFieldHostName = "hostname";
inline constexpr const char* MasterServerFieldHostPort = "hostport";
inline constexpr const char* MasterServerFieldMapName = "mapname";
inline constexpr const char* MasterServerFieldGameType = "gametype";
inline constexpr const char* MasterServerFieldNumPlayers = "numplayers";
inline constexpr const char* MasterServerFieldMaxPlayers = "maxplayers";
inline constexpr const char* MasterServerFieldGameMode = "gamemode";
inline constexpr const char* MasterServerFieldTimeLeft = "timeleft";
inline constexpr const char* MasterServerFieldStateElapsed = "stateElapsedSeconds";
inline constexpr const char* MasterServerFieldCadet = "cadet";
inline constexpr const char* MasterServerFieldDifficulty = "difficulty";
inline constexpr const char* MasterServerFieldJoinInProgress = "jip";
inline constexpr const char* MasterServerFieldDisabledAI = "disabledAI";
inline constexpr const char* MasterServerFieldRespawn = "respawn";
inline constexpr const char* MasterServerFieldRespawnDelay = "respawnDelay";
inline constexpr const char* MasterServerFieldLocked = "locked";
inline constexpr const char* MasterServerFieldDedicated = "dedicated";
inline constexpr const char* MasterServerFieldDescription = "description";
inline constexpr const char* MasterServerFieldParam1 = "param1";
inline constexpr const char* MasterServerFieldParam2 = "param2";
inline constexpr const char* MasterServerFieldRequiredAddons = "requiredAddons";
inline constexpr const char* MasterServerFieldActualVersion = "actver";
inline constexpr const char* MasterServerFieldRequiredVersion = "reqver";
inline constexpr const char* MasterServerFieldVersionTag = "vertag";
inline constexpr const char* MasterServerFieldMod = "mod";
inline constexpr const char* MasterServerFieldEqualModRequired = "equalModRequired";
inline constexpr const char* MasterServerFieldPassword = "password";
inline constexpr const char* MasterServerFieldState = "state";
inline constexpr const char* MasterServerFieldImplementation = "impl";
inline constexpr const char* MasterServerFieldPlatform = "platform";

// The only transport the remaster ships, so servers advertise this one fixed
// implementation.
inline constexpr const char* MasterServerTransportImplementation = "papa-bear";

} // namespace Poseidon
