
#include <Poseidon/Audio/Core/VoiceBudget.hpp>

#include <Poseidon/Foundation/Algorithms/Qsort.hpp>
#include <Poseidon/Foundation/Containers/StaticArray.hpp>
#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>
#include <Poseidon/Foundation/Math/MathOpt.hpp>
#include <Poseidon/Foundation/Types/RemoveLinks.hpp>

namespace Poseidon
{

namespace audio
{

namespace
{

// Internal classification helper used by ApplyVoiceBudget3D's ranking
// step.  Caches the listener-relative intensity / squared distance so
// the sort comparator runs without re-reading wave fields.
struct SortSound
{
    IWave* _wave;
    float _intensity;
    float _distance2;

    SortSound() = default;
    SortSound(IWave* wave, Vector3Val pos);
    IWave* operator->() const { return _wave; }
    operator IWave*() const { return _wave; }
    bool LouderThan(float accom, float v) const { return _intensity * accom >= v * _distance2; }
    float Loudness() const
    {
        if (_distance2 <= (_intensity * 100))
        {
            return 10.0f;
        }
        return (_intensity * 1000) / _distance2;
    }
};

} // namespace
} // namespace audio

namespace audio
{
namespace
{

SortSound::SortSound(IWave* wave, Vector3Val pos) : _wave(wave)
{
    _intensity = _wave->GetVolume();
    Vector3 wavePos = _wave->GetPosition();
    _distance2 = wavePos.Distance2(pos);
}

int CompareSounds(const SortSound* w0, const SortSound* w1)
{
    float diff = w0->_intensity * w1->_distance2 - w1->_intensity * w0->_distance2;
    if (diff > 0)
    {
        return -1;
    }
    if (diff < 0)
    {
        return +1;
    }
    return 0;
}

} // namespace

VoiceBudget3DResult ApplyVoiceBudget3D(LinkArray<IWave>& sounds, const VoiceBudget3DInput& in)
{
    VoiceBudget3DResult result;

    AUTO_STATIC_ARRAY(SortSound, sort, 128);

    // Classify every 3D wave: audible candidates go into sort, paused-
    // non-sticky take Pause() (audio-invariants A-10), everything else
    // takes Stop+Skip.
    for (int i = 0; i < sounds.Size(); i++)
    {
        IWave* wave = sounds[i];
        PoseidonAssert(wave);
        SortSound ss(wave, in.listenerPos);

        const bool pauseCheck = !in.gamePaused || wave->GetSticky();
        const bool louderCheck = ss.LouderThan(in.earAccommodation * in.soundVolume, 1e-8f);
        const bool waitCheck = !wave->IsWaiting();
        const bool termCheck = !wave->IsTerminated();

        if (pauseCheck && louderCheck && waitCheck && termCheck)
        {
            sort.Add(ss);
        }
        else if (!pauseCheck)
        {
            wave->Pause();
            ++result.pausedThisFrame;
        }
        else
        {
            wave->Stop();
            wave->Skip(in.deltaT);
        }
    }

    // Rank by loudness (intensity scaled by inverse-square distance).
    Poseidon::Foundation::QSort(sort.Data(), sort.Size(), CompareSounds);

    // Clamp to budget — audio-invariants A-11.
    int played = sort.Size();
    const int maxPlayed = in.maxAudibleBudget < kMinAudibleBudget ? kMinAudibleBudget : in.maxAudibleBudget;
    if (played > maxPlayed)
    {
        played = maxPlayed;
    }

    // Stop the overflow (waves outside the audible top-N) — audio-invariants
    // A-07: budget-eviction obeys the same replay contract as caller-Stop,
    // so the next AdvanceAll tick can re-promote them if their loudness/
    // distance changes.
    for (int i = played; i < sort.Size(); i++)
    {
        IWave* wave = sort[i];
        wave->Stop();
        wave->Skip(in.deltaT);
        ++result.evictedThisFrame;
    }

    // Play the audible top-N.  Accommodate where enabled (rolling-
    // loudness dynamic-range compression) and re-skip if Play()
    // couldn't get a channel.
    for (int i = 0; i < played; i++)
    {
        IWave* wave = sort[i];
        if (wave->AccommodationEnabled())
        {
            wave->SetAccommodation(in.earAccommodation * in.soundVolume);
            result.totalLoudness += sort[i].Loudness();
            ++result.nLoud;
        }
        wave->Play();
        if (wave->IsStopped())
        {
            wave->Skip(in.deltaT);
        }
    }
    result.audibleCount = played;
    return result;
}

Voice2DResult ApplyVoice2D(LinkArray<IWave>& sounds, const Voice2DInput& in)
{
    Voice2DResult result;

    // 2D path is uniform — no sort, no budget.  Every wave is either
    // Play+Advance (game running, or sticky) or Pause (game paused
    // and non-sticky).  Loudness from accommodation-eligible waves
    // contributes to the accommodation history.
    for (int i = 0; i < sounds.Size(); i++)
    {
        IWave* wave = sounds[i];
        // Mirror the 3D path's IsTerminated() guard.  Calling Pause() on a
        // wave that has naturally finished but whose terminated flag has
        // not yet propagated runs the pause-mid-playback path; the next
        // Play() then arms wantPlaying and DoPlay restarts the source from
        // _state.curPosition (= 0 because AL reset on natural stop).
        // User-visible symptom: any short sound that just finished
        // replays from the start on Esc-pause / Esc-unpause.
        //
        // Polling IsTerminated() here observes AL_STOPPED and sets the
        // engine flag, closing the race window for every 2D caller
        // (radio, unit voice, ambient, music).
        if (wave->IsTerminated())
            continue;
        if (wave->AccommodationEnabled())
        {
            wave->SetAccommodation(in.earAccommodation * in.soundVolume);
            const float loudness = wave->GetVolume() / Square(wave->Distance2D());
            result.totalLoudness += loudness;
        }
        if (!in.gamePaused || wave->GetSticky())
        {
            wave->Play();
            wave->Advance(in.deltaT);
            ++result.activeCount;
        }
        else
        {
            wave->Pause();
            ++result.pausedThisFrame;
        }
    }

    return result;
}

} // namespace audio

} // namespace Poseidon
