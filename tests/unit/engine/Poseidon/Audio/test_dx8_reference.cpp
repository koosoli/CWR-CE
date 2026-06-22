// test_dx8_reference.cpp -- DX8 audio math reference validation
// Verifies DX8Reference.hpp functions against values from soundDX8.cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Audio/Shared/DX8Reference.hpp>
#include <Poseidon/Audio/Shared/AudioMath.hpp>
#include <cmath>
#include <algorithm>
#include <initializer_list>

using Catch::Approx;

// ---------------------------------------------------------------------------
// expVol
// ---------------------------------------------------------------------------
TEST_CASE("DX8: expVol(10) = 1.0 (maximum)", "[Audio][DX8]")
{
    CHECK(DX8::expVol(10.f) == Approx(1.0f));
}

TEST_CASE("DX8: expVol(0) returns muted value (1e-10)", "[Audio][DX8]")
{
    // DX8 original: if (vol<=0) return 1e-10 (muted -> -200 dB)
    CHECK(DX8::expVol(0.f) == Approx(1e-10f));
}

TEST_CASE("DX8: expVol(5) ~= 0.0302", "[Audio][DX8]")
{
    CHECK(DX8::expVol(5.f) == Approx(0.0302f).margin(0.001f));
}

TEST_CASE("DX8: expVol is monotonically increasing", "[Audio][DX8]")
{
    float prev = DX8::expVol(0.f);
    for (float v = 1.f; v <= 10.f; v += 1.f)
    {
        float cur = DX8::expVol(v);
        CHECK(cur > prev);
        prev = cur;
    }
}

TEST_CASE("DX8: expVol(<=0) returns tiny value (muted)", "[Audio][DX8]")
{
    CHECK(DX8::expVol(0.f) == Approx(1e-10f));
    CHECK(DX8::expVol(-1.f) == Approx(1e-10f));
}

// ---------------------------------------------------------------------------
// float2db / db2linear roundtrip
// ---------------------------------------------------------------------------
TEST_CASE("DX8: float2db(1.0) = 0 dB", "[Audio][DX8]")
{
    CHECK(DX8::float2db(1.0f) == 0);
}

TEST_CASE("DX8: float2db(0.1) = -2000 cB", "[Audio][DX8]")
{
    CHECK(DX8::float2db(0.1f) == -2000);
}

TEST_CASE("DX8: float2db(0) = -10000 cB (min)", "[Audio][DX8]")
{
    CHECK(DX8::float2db(0.f) == -10000);
}

TEST_CASE("DX8: float2db(10.0) = 0 cB (clamped max)", "[Audio][DX8]")
{
    // log10(10)*2000 = 2000, but clamped to 0
    CHECK(DX8::float2db(10.f) == 0);
}

TEST_CASE("DX8: float2db -> db2linear roundtrip preserves gain", "[Audio][DX8]")
{
    // The dB roundtrip loses precision (integer centibels) but should be close
    for (float val : {0.001f, 0.01f, 0.1f, 0.5f, 1.0f})
    {
        int db = DX8::float2db(val);
        float back = DX8::db2linear(db);
        CHECK(back == Approx(val).margin(0.005f));
    }
}

TEST_CASE("DX8: db2linear(-10000) = 0 (muted)", "[Audio][DX8]")
{
    CHECK(DX8::db2linear(-10000) == 0.f);
}

// ---------------------------------------------------------------------------
// gain2D -- pure 2D path
// ---------------------------------------------------------------------------
TEST_CASE("DX8: gain2D unity (vol=1, accom=1, adj=1)", "[Audio][DX8]")
{
    // vol*accom*adj = 1.0, float2db(1.0) = 0, db2linear(0) = 1.0
    CHECK(DX8::gain2D(1.f, 1.f, 1.f) == Approx(1.0f).margin(0.005f));
}

TEST_CASE("DX8: gain2D with low volume", "[Audio][DX8]")
{
    // vol=0.1, accom=1, adj=0.5 -> val = 0.05
    // float2db(0.05) = log10(0.05)*2000 = -1301*2000 ~= -2602
    float g = DX8::gain2D(0.1f, 1.f, 0.5f);
    CHECK(g == Approx(0.05f).margin(0.005f));
}

TEST_CASE("DX8: gain2D with zero volume is silent", "[Audio][DX8]")
{
    CHECK(DX8::gain2D(0.f, 1.f, 1.f) == 0.f);
}

// ---------------------------------------------------------------------------
// gain2D_3DOff -- 100x boost path
// ---------------------------------------------------------------------------
TEST_CASE("DX8: gain2D_3DOff applies 100x boost", "[Audio][DX8]")
{
    // Same inputs but with 100x
    float g_2d = DX8::gain2D(0.01f, 1.f, 1.f);
    float g_3doff = DX8::gain2D_3DOff(0.01f, 1.f, 1.f);
    // 0.01 * 100 = 1.0 after boost, so 3DOff should be ~1.0
    CHECK(g_3doff == Approx(1.0f).margin(0.01f));
    // And regular 2D is 0.01
    CHECK(g_2d == Approx(0.01f).margin(0.005f));
}

TEST_CASE("DX8: gain2D_3DOff with typical values", "[Audio][DX8]")
{
    // vol=0.5, accom=0.8, adj=0.3 -> val = 0.12, * 100 = 12
    // float2db(12) = clamped to 0 -> db2linear(0) = 1.0
    float g = DX8::gain2D_3DOff(0.5f, 0.8f, 0.3f);
    CHECK(g == Approx(1.0f).margin(0.01f));
}

// ---------------------------------------------------------------------------
// gain3D -- adjust only
// ---------------------------------------------------------------------------
TEST_CASE("DX8: gain3D with volumeAdjust=1.0", "[Audio][DX8]")
{
    CHECK(DX8::gain3D(1.0f) == Approx(1.0f).margin(0.005f));
}

TEST_CASE("DX8: gain3D with small adjust", "[Audio][DX8]")
{
    float g = DX8::gain3D(0.03f);
    CHECK(g == Approx(0.03f).margin(0.005f));
}

// ---------------------------------------------------------------------------
// referenceDistance & maxDistance
// ---------------------------------------------------------------------------
TEST_CASE("DX8: referenceDistance matches AudioMath::ReferenceDistance", "[Audio][DX8]")
{
    for (float vol : {0.1f, 0.5f, 1.0f, 5.0f})
    {
        for (float accom : {0.5f, 1.0f, 2.0f})
        {
            float dx8 = DX8::referenceDistance(vol, accom);
            float oal = AudioMath::ReferenceDistance(vol, accom);
            CHECK(dx8 == Approx(oal));
        }
    }
}

TEST_CASE("DX8: referenceDistance at zero clamped to 1e-6", "[Audio][DX8]")
{
    CHECK(DX8::referenceDistance(0.f, 0.f) == Approx(1e-6f));
}

TEST_CASE("DX8: maxDistance = refDist * 100", "[Audio][DX8]")
{
    float ref = DX8::referenceDistance(1.0f, 1.0f);
    CHECK(DX8::maxDistance(ref) == Approx(ref * 100.f));
}

TEST_CASE("DX8: maxDistanceHW = 1e9", "[Audio][DX8]")
{
    CHECK(DX8::maxDistanceHW() == 1e9f);
}

// ---------------------------------------------------------------------------
// mixerAdjust
// ---------------------------------------------------------------------------
TEST_CASE("DX8: mixerAdjust -- equal sliders -> 1.0", "[Audio][DX8]")
{
    float e = DX8::expVol(5.f);
    CHECK(DX8::mixerAdjust(e, e) == Approx(1.0f));
}

TEST_CASE("DX8: mixerAdjust -- wave=10, speech=5", "[Audio][DX8]")
{
    float waveExp = DX8::expVol(10.f);
    float speechExp = DX8::expVol(5.f);
    float maxExp = std::max(waveExp, speechExp);
    CHECK(DX8::mixerAdjust(waveExp, maxExp) == Approx(1.0f));
    CHECK(DX8::mixerAdjust(speechExp, maxExp) == Approx(speechExp / waveExp));
}

TEST_CASE("DX8: mixerAdjust -- all zero -> safe (no division by zero)", "[Audio][DX8]")
{
    float e = DX8::expVol(0.f); // 1e-10
    CHECK(std::isfinite(DX8::mixerAdjust(e, e)));
}

// ---------------------------------------------------------------------------
// listenerGain
// ---------------------------------------------------------------------------
TEST_CASE("DX8: listenerGain at max (10) = 1.0", "[Audio][DX8]")
{
    CHECK(DX8::listenerGain(10.f) == Approx(1.0f));
}

TEST_CASE("DX8: listenerGain at mid (5) = 0.5", "[Audio][DX8]")
{
    CHECK(DX8::listenerGain(5.f) == Approx(0.5f));
}

TEST_CASE("DX8: listenerGain at zero = 0", "[Audio][DX8]")
{
    CHECK(DX8::listenerGain(0.f) == Approx(0.0f));
}

// ---------------------------------------------------------------------------
// Full pipeline: slider -> expVol -> adjust -> wave gain (regression guard)
// ---------------------------------------------------------------------------
TEST_CASE("DX8: full pipeline -- sliders 10/5/3, 2D wave (effect)", "[Audio][DX8]")
{
    float waveSlider = 10.f, speechSlider = 5.f, musicSlider = 3.f;

    float waveExp = DX8::expVol(waveSlider);
    float speechExp = DX8::expVol(speechSlider);
    float musicExp = DX8::expVol(musicSlider);
    float maxExp = std::max({waveExp, speechExp, musicExp});

    float effectAdj = DX8::mixerAdjust(waveExp, maxExp);
    float speechAdj = DX8::mixerAdjust(speechExp, maxExp);
    float musicAdj = DX8::mixerAdjust(musicExp, maxExp);

    // Effect has max slider -> adjust = 1.0
    CHECK(effectAdj == Approx(1.0f));
    // Speech at 5 vs max at 10 -> exp(-3.5)/1 ~= 0.030
    CHECK(speechAdj == Approx(0.0302f).margin(0.002f));

    // 2D wave: vol=0.8, accom=1.0, adj=effectAdj=1.0
    float gain = DX8::gain2D(0.8f, 1.0f, effectAdj);
    CHECK(gain == Approx(0.8f).margin(0.01f));

    // Listener gain = max(10,5,3) * 0.1 = 1.0
    CHECK(DX8::listenerGain(std::max({waveSlider, speechSlider, musicSlider})) == Approx(1.0f));
}

TEST_CASE("DX8: full pipeline -- 3D wave with distance", "[Audio][DX8]")
{
    float waveSlider = 7.f;
    float waveExp = DX8::expVol(waveSlider);
    float effectAdj = DX8::mixerAdjust(waveExp, waveExp); // only channel -> 1.0

    // 3D gain is just the adjust
    float gain = DX8::gain3D(effectAdj);
    CHECK(gain == Approx(1.0f).margin(0.005f));

    // refDist for vol=2.5, accom=0.6
    float ref = DX8::referenceDistance(2.5f, 0.6f);
    float expected = std::sqrt(2.5f * 0.6f) * 30.f;
    CHECK(ref == Approx(expected));

    // maxDist = ref * 100
    CHECK(DX8::maxDistance(ref) == Approx(expected * 100.f));
}

// ---------------------------------------------------------------------------
// Parity: OAL AudioMath vs DX8Reference
// ---------------------------------------------------------------------------
TEST_CASE("DX8: Gain2D matches AudioMath::Gain2D for normal volumes", "[Audio][DX8]")
{
    // For volumes in 0.01..1.0 range, the dB roundtrip in DX8 should
    // produce very similar results to the linear OAL gain.
    for (float vol : {0.01f, 0.05f, 0.1f, 0.3f, 0.5f, 0.8f, 1.0f})
    {
        float dx8 = DX8::gain2D(vol, 1.0f, 1.0f);
        float oal = AudioMath::Gain2D(vol, 1.0f, true, 1.0f);
        CHECK(dx8 == Approx(oal).margin(0.01f));
    }
}

TEST_CASE("DX8: Gain3D matches AudioMath::Gain3D for normal adjusts", "[Audio][DX8]")
{
    for (float adj : {0.03f, 0.1f, 0.5f, 1.0f})
    {
        float dx8 = DX8::gain3D(adj);
        float oal = AudioMath::Gain3D(adj);
        CHECK(dx8 == Approx(oal).margin(0.01f));
    }
}

// ---------------------------------------------------------------------------
// dsoundPitchRatio — 1:1 port of Wave8::SetFrequency (soundDX8.cpp:1773).
// The original mapped a pitch ratio to floor(ratio * nativeHz) clamped to the
// DirectSound rate range [100, 100000] Hz, then back to a ratio.  Replaces the
// old [0.5, 2.0] ratio clamp that floored low-RPM engine sound at 0.5x.
// ---------------------------------------------------------------------------

// Literal re-implementation of soundDX8.cpp:1773-1777 to prove 1:1 parity.
namespace
{
float DsReferencePitch(float ratio, long nativeHz, long minHz, long maxHz)
{
    long iFreq = static_cast<long>(std::floor(ratio * static_cast<float>(nativeHz)));
    if (iFreq < minHz)
        iFreq = minHz;
    else if (iFreq > maxHz)
        iFreq = maxHz;
    return static_cast<float>(iFreq) / static_cast<float>(nativeHz);
}
} // namespace

TEST_CASE("DX8: dsoundPitchRatio passes mid-range ratios through", "[Audio][DX8]")
{
    // 22 kHz sample: 0.2x .. 2.0x are all inside [100, 100000] Hz, untouched.
    CHECK(DX8::dsoundPitchRatio(1.0f, 22050) == Approx(1.0f).margin(0.001f));
    CHECK(DX8::dsoundPitchRatio(0.2f, 22050) == Approx(0.2f).margin(0.001f));
    CHECK(DX8::dsoundPitchRatio(2.0f, 22050) == Approx(2.0f).margin(0.001f));
}

TEST_CASE("DX8: dsoundPitchRatio keeps the low-RPM startup rev (fix)", "[Audio][DX8]")
{
    // The regression this guards: the old clampPitch floored every ratio at
    // 0.5x, so a low-RPM engine (e.g. rpm 0.2 -> ratio ~0.24) could never
    // drop below half pitch and the vehicle jumped straight to "full rev".
    // 0.24 * 22050 = 5292 Hz, well inside the DS range -> passes through.
    CHECK(DX8::dsoundPitchRatio(0.24f, 22050) == Approx(0.24f).margin(0.001f));
    CHECK(DX8::dsoundPitchRatio(0.24f, 22050) < 0.5f); // broken impl returned 0.5
}

TEST_CASE("DX8: dsoundPitchRatio clamps to the DirectSound rate range", "[Audio][DX8]")
{
    // Below 100 Hz absolute -> floored at 100/nativeHz.
    CHECK(DX8::dsoundPitchRatio(0.001f, 22050) == Approx(100.0f / 22050.0f).margin(1e-5f));
    // Above 100000 Hz absolute -> capped at 100000/nativeHz (~4.5x, vs old 2.0x).
    CHECK(DX8::dsoundPitchRatio(5.0f, 22050) == Approx(100000.0f / 22050.0f).margin(1e-4f));
    CHECK(DX8::dsoundPitchRatio(5.0f, 22050) > 2.0f); // broken impl capped at 2.0
}

TEST_CASE("DX8: dsoundPitchRatio is 1:1 with Wave8::SetFrequency", "[Audio][DX8]")
{
    const long rates[] = {11025, 22050, 44100};
    for (long native : rates)
    {
        for (float ratio : {0.01f, 0.1f, 0.24f, 0.5f, 1.0f, 1.7f, 2.4f, 5.0f})
        {
            CHECK(DX8::dsoundPitchRatio(ratio, native) ==
                  Approx(DsReferencePitch(ratio, native, 100, 100000)).margin(1e-6f));
        }
    }
}

TEST_CASE("DX8: dsoundPitchRatio guards degenerate input", "[Audio][DX8]")
{
    CHECK(DX8::dsoundPitchRatio(0.0f, 22050) == 1.0f);
    CHECK(DX8::dsoundPitchRatio(1.0f, 0) == 1.0f);
}
