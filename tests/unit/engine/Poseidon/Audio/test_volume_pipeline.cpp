#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Audio/Text/SoundSystemText.hpp>
#include <Poseidon/Audio/IAudioSystem.hpp>
#include <algorithm>
#include <cmath>

using namespace Poseidon;
using Catch::Approx;

// SoundSystemText provides volume getters but not UpdateMixer or volume adjust.
// These tests verify the interface contract and volume behavior at the system level.

TEST_CASE("Volume: SetWaveVolume stores and retrieves value", "[Audio][volume]")
{
    SoundSystemText sys;
    sys.SetWaveVolume(7.5f);
    CHECK(sys.GetWaveVolume() == Approx(7.5f));
}

TEST_CASE("Volume: SetSpeechVolume stores and retrieves value", "[Audio][volume]")
{
    SoundSystemText sys;
    sys.SetSpeechVolume(3.0f);
    CHECK(sys.GetSpeechVolume() == Approx(3.0f));
}

TEST_CASE("Volume: SetCDVolume stores and retrieves value", "[Audio][volume]")
{
    SoundSystemText sys;
    sys.SetCDVolume(8.2f);
    CHECK(sys.GetCDVolume() == Approx(8.2f));
}

TEST_CASE("Volume: CreateWave returns IWave with correct initial state", "[Audio][volume]")
{
    SoundSystemText sys;
    IWave* wave = sys.CreateWave("test.wav", false, false);
    REQUIRE(wave != nullptr);

    // Wave should start with default volume
    CHECK(wave->GetVolume() == Approx(1.0f).margin(0.01f));
    CHECK(!wave->IsTerminated());
    CHECK(wave->IsStopped());
    CHECK(wave->AccommodationEnabled());

    wave->Release();
}

TEST_CASE("Volume: Wave SetVolume updates GetVolume", "[Audio][volume]")
{
    SoundSystemText sys;
    IWave* wave = sys.CreateWave("test.wav", false, false);
    REQUIRE(wave != nullptr);

    wave->SetVolume(0.5f, 1.0f, true);
    CHECK(wave->GetVolume() == Approx(0.5f));

    wave->SetVolume(0.0f, 1.0f, true);
    CHECK(wave->GetVolume() == Approx(0.0f));

    wave->SetVolume(2.0f, 1.0f, true);
    CHECK(wave->GetVolume() == Approx(2.0f));

    wave->Release();
}

TEST_CASE("Volume: Wave WaveKind affects category", "[Audio][volume]")
{
    SoundSystemText sys;
    IWave* wave = sys.CreateWave("test.wav", false, false);
    REQUIRE(wave != nullptr);

    CHECK(wave->GetKind() == WaveEffect); // default
    wave->SetKind(WaveSpeech);
    CHECK(wave->GetKind() == WaveSpeech);
    wave->SetKind(WaveMusic);
    CHECK(wave->GetKind() == WaveMusic);

    wave->Release();
}

TEST_CASE("Volume: Accommodation can be enabled/disabled", "[Audio][volume]")
{
    SoundSystemText sys;
    IWave* wave = sys.CreateWave("test.wav", false, false);
    REQUIRE(wave != nullptr);

    CHECK(wave->AccommodationEnabled());
    wave->EnableAccommodation(false);
    CHECK(!wave->AccommodationEnabled());
    wave->EnableAccommodation(true);
    CHECK(wave->AccommodationEnabled());

    wave->SetAccommodation(0.7f);
    CHECK(wave->GetAccommodation() == Approx(0.7f));

    wave->Release();
}

TEST_CASE("Volume: 3D wave flag", "[Audio][volume]")
{
    SoundSystemText sys;

    IWave* wave3D = sys.CreateWave("test.wav", true, false);
    REQUIRE(wave3D != nullptr);
    CHECK(wave3D->Get3D());
    wave3D->Release();

    IWave* wave2D = sys.CreateWave("test.wav", false, false);
    REQUIRE(wave2D != nullptr);
    CHECK(!wave2D->Get3D());
    wave2D->Release();
}

TEST_CASE("Volume: Wave play/stop lifecycle", "[Audio][volume]")
{
    SoundSystemText sys;
    IWave* wave = sys.CreateWave("test.wav", false, false);
    REQUIRE(wave != nullptr);

    CHECK(wave->IsStopped());
    wave->Play();
    CHECK(!wave->IsStopped());
    wave->Stop();
    CHECK(wave->IsStopped());

    wave->Release();
}

TEST_CASE("Volume: StartPreview and TerminatePreview don't crash", "[Audio][volume]")
{
    SoundSystemText sys;
    sys.StartPreview();     // should not crash
    sys.TerminatePreview(); // should not crash
    sys.TerminatePreview(); // double terminate should be safe
}

// expVol curve tests: verify the volume curve matching DX8 reference
// expVol(vol) = exp((vol - 10) * 0.7) (soundDX8.cpp:2911)
static inline float testExpVol(float vol)
{
    return expf((vol - 10.f) * 0.7f);
}

TEST_CASE("Volume: expVol curve values match DX8 reference", "[Audio][volume]")
{
    // vol=10 -> 1.0 (maximum)
    CHECK(testExpVol(10.f) == Approx(1.0f));
    // vol=0 -> exp(-7) ≈ 9.12e-4
    CHECK(testExpVol(0.f) == Approx(expf(-7.0f)));
    // vol=5 -> exp(-3.5) ≈ 0.0302
    CHECK(testExpVol(5.f) == Approx(0.0302f).margin(0.001f));
    // vol=3 -> exp(-4.9) ≈ 0.00745
    CHECK(testExpVol(3.f) == Approx(expf(-4.9f)));
    // monotonically increasing
    CHECK(testExpVol(1.f) < testExpVol(3.f));
    CHECK(testExpVol(3.f) < testExpVol(5.f));
    CHECK(testExpVol(5.f) < testExpVol(7.f));
    CHECK(testExpVol(7.f) < testExpVol(10.f));
}

TEST_CASE("Volume: mixer adjusts -- DX8 formula", "[Audio][volume]")
{
    // DX8: adjusts = expVol(ch) / max(all expVols), no floor
    // (soundDX8.cpp:2977-2979)

    // All sliders equal at 5 -> all adjusts = 1.0
    float allExp = testExpVol(5.f);
    float maxVolExp = std::max({allExp, allExp, allExp});
    CHECK(allExp / maxVolExp == Approx(1.0f));

    // All at 3 -> still all adjusts = 1.0 (equal sliders)
    float allExp3 = testExpVol(3.f);
    float maxVolExp3 = std::max({allExp3, allExp3, allExp3});
    CHECK(allExp3 / maxVolExp3 == Approx(1.0f));
}

TEST_CASE("Volume: mixer adjusts with wave=10 speech=5", "[Audio][volume]")
{
    float waveExp = testExpVol(10.f);
    float speechExp = testExpVol(5.f);
    float musicExp = testExpVol(5.f);
    float maxVolExp = std::max({waveExp, speechExp, musicExp});
    // wave at max -> adjust = 1.0
    CHECK(waveExp / maxVolExp == Approx(1.0f));
    // speech at 5 -> adjust ≈ 0.0302
    CHECK(speechExp / maxVolExp == Approx(0.0302f).margin(0.001f));
}

TEST_CASE("Volume: listener gain matches DX8 hardware mixer", "[Audio][volume]")
{
    // DX8: _mixer->SetWaveVolume(MaxVolume() * 0.1)
    // MaxVolume() = max(raw slider values) in 0-10 range
    float waveVol = 5.f, speechVol = 7.f, cdVol = 3.f;
    float maxRawVol = std::max({waveVol, speechVol, cdVol});
    float listenerGain = maxRawVol * 0.1f;
    CHECK(listenerGain == Approx(0.7f));

    // At default (all 5): gain = 0.5
    CHECK(std::max({5.f, 5.f, 5.f}) * 0.1f == Approx(0.5f));

    // At max (all 10): gain = 1.0
    CHECK(std::max({10.f, 10.f, 10.f}) * 0.1f == Approx(1.0f));

    // At min (all 0): gain = 0.0
    CHECK(std::max({0.f, 0.f, 0.f}) * 0.1f == Approx(0.0f));
}

// ---------------------------------------------------------------------------
// DX8 parity: 3-path volume logic
// ---------------------------------------------------------------------------

#include <Poseidon/Audio/Shared/DX8Reference.hpp>
#include <Poseidon/Audio/Shared/AudioMath.hpp>

TEST_CASE("Volume: 3D wave Set3D(false) triggers boost path", "[Audio][volume][DX8]")
{
    // In DX8, a wave created as 3D but with Set3D(false) gets 100x volume boost.
    // Verify the DX8 reference models this.
    float vol = 0.01f, accom = 1.f, adj = 0.5f;
    float plain2D = DX8::gain2D(vol, accom, adj);
    float boosted = DX8::gain2D_3DOff(vol, accom, adj);
    // 100x boost: 0.01*1*0.5*100 = 0.5, should be much larger than 0.005
    CHECK(boosted > plain2D * 10.f);
}

TEST_CASE("Volume: 3DOff boost clamps at gain 1.0", "[Audio][volume][DX8]")
{
    // If vol*accom*adj*100 > 1.0, DX8 float2db clamps at 0 dB = gain 1.0
    float g = DX8::gain2D_3DOff(0.5f, 1.f, 1.f); // 0.5*100 = 50, clamped
    CHECK(g == Approx(1.f).margin(0.01f));
}

TEST_CASE("Volume: pure 2D and 3D match AudioMath", "[Audio][volume][DX8]")
{
    // For normal volumes, DX8 dB roundtrip ≈ linear
    float vol = 0.3f, accom = 0.8f, adj = 0.6f;
    float dx8_2d = DX8::gain2D(vol, accom, adj);
    float oal_2d = AudioMath::Gain2D(vol, accom, true, adj);
    CHECK(dx8_2d == Approx(oal_2d).margin(0.01f));

    float dx8_3d = DX8::gain3D(adj);
    float oal_3d = AudioMath::Gain3D(adj);
    CHECK(dx8_3d == Approx(oal_3d).margin(0.01f));
}

TEST_CASE("Volume: mixer pipeline regression guard", "[Audio][volume][DX8]")
{
    // Fixed regression values — if DX8Reference changes, these break
    CHECK(DX8::expVol(10.f) == Approx(1.0f));
    CHECK(DX8::expVol(5.f) == Approx(0.030197f).margin(0.0005f));
    CHECK(DX8::listenerGain(7.f) == Approx(0.7f));
    CHECK(DX8::referenceDistance(1.f, 1.f) == Approx(30.f));
    CHECK(DX8::maxDistance(30.f) == Approx(3000.f));
}
