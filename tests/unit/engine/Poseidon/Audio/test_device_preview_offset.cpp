// Unit tests for audio::DevicePreviewOffsetSeconds — the pure wall-
// clock anchor math behind SoundSystemOAL::StartDevicePreview.
// Covers audio-invariants A-17.
//
// The math is: given an anchor wall-clock timestamp (ms) and current
// wall-clock (ms), compute the resume offset (seconds) into a wave of
// known length such that the playback head advances continuously
// across rapid SwitchOutputDevice cycles.
//
// Background: B-A14 — device-picker preview restarting from 0 each
// click felt jumpy.  Wall-clock anchoring stays continuous because
// elapsed time between clicks is small relative to wave length, so
// the modulo wraps smoothly.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Audio/Core/WaveLifecycle.hpp>
#include <stdint.h>

using namespace Poseidon;
using Catch::Approx;

TEST_CASE("DevicePreviewOffset: zero anchor -> first-call offset is 0", "[audio][A-17]")
{
    // anchor=0 is the "not yet started" sentinel.  Result must be 0 so
    // the wave plays from the start on the first StartDevicePreview.
    CHECK(audio::DevicePreviewOffsetSeconds(/*now=*/12345u, /*anchor=*/0u, /*length=*/10.f) == 0.f);
}

TEST_CASE("DevicePreviewOffset: zero length -> 0", "[audio][A-17]")
{
    // Defensive: wave length unknown / unloaded -> can't seek meaningfully.
    CHECK(audio::DevicePreviewOffsetSeconds(1000u, 500u, 0.f) == 0.f);
    CHECK(audio::DevicePreviewOffsetSeconds(1000u, 500u, -1.f) == 0.f);
}

TEST_CASE("DevicePreviewOffset: elapsed < length -> direct offset", "[audio][A-17]")
{
    // anchor at t=1000ms, now at t=3500ms, length=10s -> elapsed=2.5s,
    // 2.5 mod 10 = 2.5.
    CHECK(audio::DevicePreviewOffsetSeconds(3500u, 1000u, 10.f) == Approx(2.5f));
}

TEST_CASE("DevicePreviewOffset: elapsed > length -> wraps via modulo", "[audio][A-17]")
{
    // anchor=0 is reserved for "first call" sentinel; non-zero anchors
    // exercise the modulo path.
    // anchor at t=1ms, now at t=12501ms, length=10s -> elapsed=12.5s,
    // 12.5 mod 10 = 2.5.
    CHECK(audio::DevicePreviewOffsetSeconds(12501u, 1u, 10.f) == Approx(2.5f));
    // Multi-wrap: elapsed=23s, length=10s -> 3s.
    CHECK(audio::DevicePreviewOffsetSeconds(23001u, 1u, 10.f) == Approx(3.f));
}

TEST_CASE("DevicePreviewOffset: wall-clock wrap is harmless", "[audio][A-17]")
{
    // GlobalTickCount() is uint32_t milliseconds; wraps every ~49.7
    // days.  If now < anchor (the wrap case), return 0 instead of a
    // bogus huge offset.  Rare in practice but defensive.
    CHECK(audio::DevicePreviewOffsetSeconds(100u, 4'000'000'000u, 10.f) == 0.f);
}

TEST_CASE("DevicePreviewOffset: continuity across rapid restarts", "[audio][A-17]")
{
    // Simulate two StartDevicePreview calls 100ms apart on a 5s wave.
    // First call anchors at t=1000ms — but on the first call, we'd
    // typically pass anchor=0 to indicate "first start" (returns 0).
    // The caller then records the anchor.  Second call passes the
    // recorded anchor=1000 with now=1100, returning 0.1s.  Third
    // call at now=1200 returns 0.2s — continuous progression.
    const uint32_t anchor = 1000u;
    const float length = 5.f;
    CHECK(audio::DevicePreviewOffsetSeconds(1100u, anchor, length) == Approx(0.1f));
    CHECK(audio::DevicePreviewOffsetSeconds(1200u, anchor, length) == Approx(0.2f));
    CHECK(audio::DevicePreviewOffsetSeconds(2000u, anchor, length) == Approx(1.0f));
}
