// Portable DX8 audio math reference — pure functions mirroring the original
// soundDX8.cpp, used by the OpenAL backend for 1:1 parity and by tests.
#pragma once

#include <cmath>
#include <algorithm>

namespace DX8
{

// Volume exponential curve (soundDX8.cpp:2911)
// Maps slider value 0..10 to exponential gain
// vol=10 -> 1.0, vol=0 -> ~9.1e-4, vol=5 -> ~0.030
inline float expVol(float vol)
{
    if (vol <= 0.f)
        return 1e-10f; // muted -> effectively silent
    return std::exp((vol - 10.f) * 0.7f);
}

// DirectSound centibels conversion (soundDX8.cpp:1601)
// log10(val)*2000, clamped to [-10000, 0]
// Only needed for DS API — OpenAL uses linear gain directly.
inline int float2db(float val)
{
    if (val <= 0.f)
        return -10000;
    int ret = static_cast<int>(std::floor(std::log10(val) * 2000.f));
    if (ret < -10000)
        ret = -10000;
    if (ret > 0)
        ret = 0;
    return ret;
}

// Effective linear gain from centibels (inverse of float2db)
// DS stores volume as centibels then converts back to linear for mixing.
// gain = 10^(db/2000)
inline float db2linear(int db)
{
    if (db <= -10000)
        return 0.f;
    return std::pow(10.f, db / 2000.f);
}

// 2D volume — pure 2D path (soundDX8.cpp:1716)
// vol * accom * adjust -> centibels -> linear
// The dB roundtrip clamps at -100dB (effectively 0).
inline float gain2D(float volume, float accommodation, float volumeAdjust)
{
    float val = volume * accommodation * volumeAdjust;
    return db2linear(float2db(val));
}

// 2D volume — 3D buffer with 3D disabled (soundDX8.cpp:1717-1718)
// Used when Set3D(false) is called on a 3D-capable sound (e.g., inside vehicle).
// DX8 applies 100x boost to compensate for DirectSound 3D buffer attenuation.
// In OpenAL, sources don't have this implicit 3D-buffer attenuation, so the
// effective boost compensates for the difference in distance rolloff that would
// normally apply to a 3D source that's now heard as 2D.
inline float gain2D_3DOff(float volume, float accommodation, float volumeAdjust)
{
    float val = volume * accommodation * volumeAdjust * 100.f;
    return db2linear(float2db(val));
}

// 3D volume — only adjust (soundDX8.cpp:1721-1722)
// Volume is encoded in MinDistance, so direct buffer volume is just the adjust.
inline float gain3D(float volumeAdjust)
{
    return db2linear(float2db(volumeAdjust));
}

// 3D reference distance (= DirectSound MinDistance) (soundDX8.cpp:1640)
// sqrt(vol * accom) * 30, minimum 1e-6
inline float referenceDistance(float volume, float accommodation)
{
    float d = std::sqrt(volume * accommodation) * 30.f;
    return d < 1e-6f ? 1e-6f : d;
}

// 3D max distance — SW path (soundDX8.cpp:1651)
// Modern systems: minDist * 100 (no HW acceleration)
inline float maxDistance(float refDist)
{
    return refDist * 100.f;
}

// 3D max distance — HW path (soundDX8.cpp:1645)
// Legacy SB Live/Audigy: DS3D_DEFAULTMAXDISTANCE = 1e9
inline float maxDistanceHW()
{
    return 1e9f;
}

// Mixer: per-channel volume adjust (soundDX8.cpp:2977-2979)
// ratio = expVol(channel) / max(expVol(all channels))
inline float mixerAdjust(float channelExp, float maxExp)
{
    if (maxExp < 1e-20f)
        maxExp = 1e-20f;
    return channelExp / maxExp;
}

// Mixer: listener/master gain (soundDX8.cpp:3000)
// DX8 calls waveOutSetVolume(maxSlider * 0.1) — OS-level 0..1 wave device gain.
// OpenAL equivalent: alListenerf(AL_GAIN, maxSlider * 0.1)
inline float listenerGain(float maxRawSlider)
{
    return maxRawSlider * 0.1f;
}

// 1:1 port of 1.99 Wave8::SetFrequency (soundDX8.cpp:1773).  The engine
// drives pitch as a ratio `freqRatio` (1.0 = native rate) — e.g. a vehicle
// engine sound uses freqRatio = rpm * 1.2.  DirectSound set the ABSOLUTE
// playback rate to floor(freqRatio * nativeHz), clamped to the secondary-
// buffer range [minHz, maxHz] (DSBFREQUENCY fallbacks 100 / 100000 Hz).
// OpenAL expects a pitch RATIO, so we divide the clamped rate back by
// nativeHz.  This preserves the original's wide rev range (≈0.005x .. 4.5x
// for a 22 kHz sample) so a low-RPM engine keeps its deep startup pitch
// instead of being floored at 0.5x — the bug that made vehicles jump
// straight to "full rev".
inline float dsoundPitchRatio(float freqRatio, long nativeFreqHz, long minHz = 100, long maxHz = 100000)
{
    if (nativeFreqHz <= 0 || freqRatio <= 0.f)
        return 1.0f; // degenerate input — leave pitch unchanged
    long iFreq = static_cast<long>(std::floor(freqRatio * static_cast<float>(nativeFreqHz)));
    if (iFreq < minHz)
        iFreq = minHz;
    else if (iFreq > maxHz)
        iFreq = maxHz;
    return static_cast<float>(iFreq) / static_cast<float>(nativeFreqHz);
}

} // namespace DX8

