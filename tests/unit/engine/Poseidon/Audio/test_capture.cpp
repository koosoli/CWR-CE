// Tests for SoundSystemCapture + WaveCapture buffer-capture backend
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Audio/Capture/SoundSystemCapture.hpp>
#include <Poseidon/Audio/Capture/WaveCapture.hpp>
#include <Poseidon/Audio/Shared/AudioMath.hpp>
#include <cmath>
#include <numeric>
#include <string.h>
#include <format>
#include <vector>
#include <Poseidon/Foundation/Strings/RString.hpp>

using namespace Poseidon;
using Catch::Approx;

static float rms(const std::vector<float>& v)
{
    if (v.empty())
        return 0.f;
    float sum = 0.f;
    for (float s : v)
        sum += s * s;
    return std::sqrt(sum / static_cast<float>(v.size()));
}

TEST_CASE("SoundSystemCapture creates waves", "[Audio][capture]")
{
    SoundSystemCapture sys;
    auto* w = dynamic_cast<WaveCapture*>(sys.CreateWave("test.wav", false));
    REQUIRE(w != nullptr);
    CHECK(strcmp(w->Name(), "test.wav") == 0);
    CHECK(w->IsStopped());
    CHECK(sys.GetWaves().size() == 1);
}

TEST_CASE("WaveCapture renders silence when stopped", "[Audio][capture]")
{
    SoundSystemCapture sys;
    auto* w = dynamic_cast<WaveCapture*>(sys.CreateWave("test.wav", false));
    // Don't play - just render
    sys.RenderAll(100);
    CHECK(w->GetCapturedSamples().empty());
}

TEST_CASE("WaveCapture renders signal when playing", "[Audio][capture]")
{
    SoundSystemCapture sys;
    auto* w = dynamic_cast<WaveCapture*>(sys.CreateWave("test.wav", false));
    w->SetVolume(1.f);
    w->Play();
    sys.RenderAll(100);
    auto& samples = w->GetCapturedSamples();
    REQUIRE(samples.size() == 100);
    CHECK(rms(samples) > 0.f);
}

TEST_CASE("WaveCapture volume 0 = silence", "[Audio][capture]")
{
    SoundSystemCapture sys;
    auto* w = dynamic_cast<WaveCapture*>(sys.CreateWave("test.wav", false));
    w->SetVolume(0.f);
    w->Play();
    sys.RenderAll(50);
    CHECK(rms(w->GetCapturedSamples()) == 0.f);
}

TEST_CASE("WaveCapture volume scaling", "[Audio][capture]")
{
    SoundSystemCapture sys;
    auto* w = dynamic_cast<WaveCapture*>(sys.CreateWave("test.wav", false));
    w->SetVolume(0.5f);
    w->Play();
    sys.RenderAll(50);
    float halfRms = rms(w->GetCapturedSamples());

    w->ClearCapture();
    w->SetVolume(1.f);
    sys.RenderAll(50);
    float fullRms = rms(w->GetCapturedSamples());

    CHECK(fullRms > halfRms);
}

TEST_CASE("WaveCapture accommodation multiplier", "[Audio][capture]")
{
    SoundSystemCapture sys;
    auto* w = dynamic_cast<WaveCapture*>(sys.CreateWave("test.wav", false));
    w->SetVolume(1.f);
    w->SetAccommodation(0.5f);
    w->EnableAccommodation(true);
    w->Play();
    sys.RenderAll(50);
    float withAccom = rms(w->GetCapturedSamples());

    w->ClearCapture();
    w->EnableAccommodation(false);
    sys.RenderAll(50);
    float withoutAccom = rms(w->GetCapturedSamples());

    CHECK(withoutAccom > withAccom);
}

TEST_CASE("WaveCapture stop produces no more samples", "[Audio][capture]")
{
    SoundSystemCapture sys;
    auto* w = dynamic_cast<WaveCapture*>(sys.CreateWave("test.wav", false));
    w->SetVolume(1.f);
    w->Play();
    sys.RenderAll(20);
    REQUIRE(w->GetCapturedSamples().size() == 20);
    w->Stop();
    sys.RenderAll(20);
    CHECK(w->GetCapturedSamples().size() == 20); // No new samples after stop
}

TEST_CASE("WaveCapture 3D uses volumeAdjust only", "[Audio][capture]")
{
    SoundSystemCapture sys;
    auto* w = dynamic_cast<WaveCapture*>(sys.CreateWave("test.wav", true));
    w->SetVolume(0.5f); // Should be ignored for 3D gain
    w->Play();
    sys.RenderAll(10);
    float gain3d = rms(w->GetCapturedSamples());
    // 3D gain = volumeAdjust = waveVolume (set by system) = 1.0
    CHECK(gain3d == Approx(1.f));
}

TEST_CASE("SoundSystemCapture multiple waves mix independently", "[Audio][capture]")
{
    SoundSystemCapture sys;
    auto* w1 = dynamic_cast<WaveCapture*>(sys.CreateWave("a.wav", false));
    auto* w2 = dynamic_cast<WaveCapture*>(sys.CreateWave("b.wav", false));
    w1->SetVolume(1.f);
    w2->SetVolume(0.5f);
    w1->Play();
    w2->Play();
    sys.RenderAll(30);
    CHECK(rms(w1->GetCapturedSamples()) > rms(w2->GetCapturedSamples()));
}

TEST_CASE("SoundSystemCapture system volume affects output", "[Audio][capture]")
{
    SoundSystemCapture sys;
    auto* w = dynamic_cast<WaveCapture*>(sys.CreateWave("test.wav", false));
    w->SetVolume(1.f);
    w->Play();

    sys.SetWaveVolume(1.f);
    sys.RenderAll(30);
    float fullRms = rms(w->GetCapturedSamples());

    w->ClearCapture();
    sys.SetWaveVolume(0.5f);
    sys.RenderAll(30);
    float halfRms = rms(w->GetCapturedSamples());

    CHECK(fullRms > halfRms);
}

TEST_CASE("WaveCapture restart clears capture", "[Audio][capture]")
{
    SoundSystemCapture sys;
    auto* w = dynamic_cast<WaveCapture*>(sys.CreateWave("test.wav", false));
    w->SetVolume(1.f);
    w->Play();
    sys.RenderAll(10);
    REQUIRE(!w->GetCapturedSamples().empty());
    w->Restart();
    CHECK(w->GetCapturedSamples().empty());
    CHECK(w->IsStopped());
}
