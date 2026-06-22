// Lifecycle helpers for cached IWave* reuse — audio-invariants A-13.
//
// Gameplay code that holds a cached IWave* across frames must NOT treat
// non-null as "playable".  Budget eviction, terminal completion, and load
// failure can all leave a non-null wave that no longer plays.  These
// helpers consult IWave::State() so the cache-vs-lifecycle distinction
// stays one explicit check instead of a scatter of IsTerminated() /
// IsStopped() probes.
//
// Use:
//   IWave* cached = _weaponSound[slot];
//   if (audio::IsCacheStale(cached)) {
//       _weaponSound[slot] = nullptr;
//       cached = GSoundScene->OpenAndPlay(...);
//   } else {
//       audio::RetriggerCachedWave(cached);
//   }

#pragma once

#include <Poseidon/Audio/IAudioSystem.hpp>

#include <cmath>
#include <cstdint>


namespace Poseidon
{
namespace audio
{

// True if the cached wave can still be re-used — non-null and not in
// the terminal state.
inline bool IsCacheValid(IWave* wave)
{
    return wave != nullptr && wave->State() != WaveState::Terminal;
}

// True if the cached wave should be dropped — null or terminal.
// Cleaner phrasing of !IsCacheValid for the common "drop the cache"
// idiom.
inline bool IsCacheStale(IWave* wave)
{
    return !IsCacheValid(wave);
}

// Retrigger a cached wave so its next Commit produces audible playback,
// regardless of prior state.  Returns true if the wave is now in a
// playable lifecycle position (Playing / WaitingDeferred / Paused→
// Resume'd / StoppedReplayable→Play'd / Created→Play'd).  Returns false
// only if the wave is null or Terminal — in which case the caller
// must drop the cache entry and load a fresh wave.
//
// Behavior by prior state:
//   Created           -> Play() — wave starts from offset 0
//   WaitingDeferred   -> no-op — Commit will run DoPlay
//   Playing           -> no-op — already active
//   Paused            -> Resume() — wave continues from stored offset
//   StoppedReplayable -> Play() — replays from current cursor
//   Terminal          -> false  (caller drops and reloads)
inline bool RetriggerCachedWave(IWave* wave)
{
    if (!wave)
    {
        return false;
    }
    switch (wave->State())
    {
        case WaveState::Terminal:
            return false;
        case WaveState::Playing:
        case WaveState::WaitingDeferred:
            return true;
        case WaveState::Paused:
            wave->Resume();
            return true;
        case WaveState::Created:
        case WaveState::StoppedReplayable:
            wave->Play();
            return true;
    }
    return false; // unreachable
}

// Device-picker preview wall-clock offset math — audio-invariants A-17.
// The Output Device picker plays a long-form preview wave looped while
// the row is focused.  Repeated StartDevicePreview calls (one per row
// focus change / SwitchOutputDevice) compute the resume offset from
// (now_ms - anchor_ms) mod length_sec so the playback head advances
// continuously instead of restarting at 0 each click.
//
// Pure function — no I/O, no AL state.  Tested in
// tests/unit/engine/Poseidon/Audio/test_device_preview_offset.cpp.
inline float DevicePreviewOffsetSeconds(uint32_t nowMs, uint32_t anchorMs, float lengthSec)
{
    if (lengthSec <= 0.f)
    {
        return 0.f;
    }
    if (anchorMs == 0 || nowMs < anchorMs)
    {
        // First call (anchor not set) or wall-clock wrap — restart at 0.
        return 0.f;
    }
    const float elapsedSec = static_cast<float>(nowMs - anchorMs) / 1000.f;
    float result = std::fmod(elapsedSec, lengthSec);
    if (result < 0.f)
    {
        result += lengthSec;
    }
    return result;
}

} // namespace audio

} // namespace Poseidon
