#pragma once

// Engine runtime state (not user configuration): visibility distances and computed
// fields owned by the engine. Not serialized to config files, not shared with launcher.

namespace Poseidon
{
struct EngineState
{
    // Visibility distances (set by engine from LOD/terrain)
    float horizontZ = 900.0f;
    float objectsZ = 600.0f;
    float tacticalZ = 900.0f;
    float radarZ = 2500.0f;
    float shadowsZ = 250.0f;

    // Computed gameplay fields.
    // Track decals (jeep tires, tank tracks) decay linearly from spawn
    // alpha to 0 over `trackTimeToLive` seconds; tracks.cpp multiplies
    // deltaT by `invTrackTimeToLive` each frame.  Leaving them at 0 makes
    // tracks permanent and forces a new shape every frame (see tracks.cpp:461),
    // so default to 60s / (1/60).
    float trackTimeToLive = 60.0f;
    float invTrackTimeToLive = 1.0f / 60.0f;
};
} // namespace Poseidon
