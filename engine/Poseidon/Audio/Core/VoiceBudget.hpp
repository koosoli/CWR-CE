// 3D + 2D voice-budget driver — audio-invariants A-07 / A-08 / A-09 / A-11.
//
// Owns the policy that decides which 3D waves render audibly each
// frame, which get paused (game-pause path), and which get evicted
// (budget overflow).  Driven from SoundScene::AdvanceAll.

#pragma once

#include <Poseidon/Audio/IAudioSystem.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Core/HandledList.hpp>


namespace Poseidon
{
namespace audio
{

// audio-invariants A-11 — the audible budget floors at 16.  Used
// exclusively from ApplyVoiceBudget3D so a misconfigured
// ENGINE_CONFIG.maxSounds cannot silently produce a sub-16 budget
// that mass-evicts gameplay sounds.
constexpr int kMinAudibleBudget = 16;

struct VoiceBudget3DInput
{
    Vector3 listenerPos;
    float earAccommodation = 1.f;
    float soundVolume = 1.f;
    // Caller passes ENGINE_CONFIG.maxSounds verbatim; ApplyVoiceBudget3D
    // applies the kMinAudibleBudget floor internally so all paths see
    // the same effective budget.
    int maxAudibleBudget = kMinAudibleBudget;
    bool gamePaused = false;
    float deltaT = 0.f;
};

// Snapshot of the budget decisions taken this frame.  Tests / counters /
// crowded-scene diagnostics (audio-invariants A-28 / A-30) read this
// to verify the eviction matched the expected ranking.
struct VoiceBudget3DResult
{
    int audibleCount = 0;        // top-N rendered audibly
    int evictedThisFrame = 0;    // budget-stopped this tick (rank > N + inaudible-quiet + waiting + terminated)
    int pausedThisFrame = 0;     // paused via Pause() because gamePaused && !sticky
    float totalLoudness = 0.f;   // sum of audible loudness (accommodation history input)
    int nLoud = 0;               // count of accommodating audible waves
};

// Apply the 3D voice budget to a set of waves.  Mutates each wave via
// Play / Stop / Pause / Skip / SetAccommodation directly.  Bit-equal
// for every input combination (audible top-N, paused-non-sticky,
// budget-evicted, inaudible-quiet, waiting, terminated).
VoiceBudget3DResult ApplyVoiceBudget3D(LinkArray<IWave>& sounds, const VoiceBudget3DInput& input);

// audio-invariants A-08 — 2D and 3D paths are intentionally distinct.
// The 2D loop has no budget / no distance ranking; every wave is
// either Play'd (game running, or sticky) or Pause'd (game paused
// and non-sticky).  Loudness from accommodation-enabled waves
// accumulates into totalLoudness for the accommodation-history step.
struct Voice2DInput
{
    float earAccommodation = 1.f;
    float soundVolume = 1.f;
    bool gamePaused = false;
    float deltaT = 0.f;
};

struct Voice2DResult
{
    int activeCount = 0;         // waves that took the Play+Advance path
    int pausedThisFrame = 0;     // waves that took the Pause path
    float totalLoudness = 0.f;   // sum of accommodation-eligible loudness
};

Voice2DResult ApplyVoice2D(LinkArray<IWave>& sounds, const Voice2DInput& input);

} // namespace audio

} // namespace Poseidon
