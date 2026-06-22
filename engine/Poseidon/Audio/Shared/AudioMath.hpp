#pragma once

#include <cmath>

namespace AudioMath
{

// 3D reference distance from volume and accommodation
// Used by OpenAL (AL_REFERENCE_DISTANCE)
// Origin: DirectSound MinDistance = sqrt(vol * accom) * 30
inline float ReferenceDistance(float volume, float accommodation)
{
    float d = std::sqrt(volume * accommodation) * 30.f;
    return d < 1e-6f ? 1e-6f : d;
}

// Max audible distance (100x reference, matching DirectSound MaxDistance)
inline float MaxDistance(float volume, float accommodation)
{
    return ReferenceDistance(volume, accommodation) * 100.f;
}

// 2D gain: volume * accommodation * categoryAdjust
// Clamped to [0, 1] matching DX8 dB roundtrip (float2db clamps at 0 dB)
inline float Gain2D(float volume, float accommodation, bool accomEnabled, float volumeAdjust)
{
    float g = volume;
    if (accomEnabled)
        g *= accommodation;
    g *= volumeAdjust;
    if (g <= 0.f)
        return 0.f;
    if (g >= 1.f)
        return 1.f;
    return g;
}

// 3D gain: category adjust only (distance handles volume via 3D curves)
// Match DX8 dB roundtrip: float2db clamps to 0 dB for values >= 1.0
inline float Gain3D(float volumeAdjust)
{
    if (volumeAdjust <= 0.f)
        return 0.f;
    if (volumeAdjust >= 1.f)
        return 1.f;
    return volumeAdjust;
}

} // namespace AudioMath

