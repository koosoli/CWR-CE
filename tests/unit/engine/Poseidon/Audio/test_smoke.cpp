// Backend smoke tests: factory creation, nosound fallback, capture backend
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Audio/AudioFactory.hpp>
#include <Poseidon/Audio/IAudioSystem.hpp>
#include <Poseidon/Audio/Capture/SoundSystemCapture.hpp>
#include <Poseidon/Audio/Capture/WaveCapture.hpp>
#include <Poseidon/Audio/Dummy/SoundSystemDummy.hpp>
#include <string>
#include <vector>

using namespace Poseidon;
TEST_CASE("AudioFactory: Dummy backend creates valid system", "[Audio][factory]")
{
    AudioCreateResult created = AudioFactory::Create("dummy", false);
    auto* sys = created.system;
    REQUIRE(sys != nullptr);
    CHECK(std::string(created.backend.codeName) == "dummy");
    CHECK(sys->GetWaveVolume() >= 0.f);
    auto* w = sys->CreateWave("test.wav", false);
    REQUIRE(w != nullptr);
    w->Play();
    CHECK(!w->IsTerminated());
    w->Stop();
    w->Release();
    delete sys;
}

TEST_CASE("AudioFactory: noSound flag creates working system", "[Audio][factory]")
{
    AudioCreateResult created = AudioFactory::Create("auto", true);
    auto* sys = created.system;
    REQUIRE(sys != nullptr);
    CHECK(std::string(created.backend.codeName) == "dummy");
    auto* w = sys->CreateWave("nonexistent.wav", false);
    REQUIRE(w != nullptr);
    w->Release();
    delete sys;
}

TEST_CASE("AudioFactory: explicit backend selection overrides noSound", "[Audio][factory][registry]")
{
    AudioCreateResult created = AudioFactory::Create("text", true);
    auto* sys = created.system;
    REQUIRE(sys != nullptr);
    CHECK(std::string(created.backend.codeName) == "text");
    delete sys;
}

TEST_CASE("AudioFactory: Text backend creates valid system", "[Audio][factory]")
{
    AudioCreateResult created = AudioFactory::Create("text", false);
    auto* sys = created.system;
    REQUIRE(sys != nullptr);
    CHECK(std::string(created.backend.codeName) == "text");
    auto* w = sys->CreateWave("test.wav", false);
    REQUIRE(w != nullptr);
    w->Play();
    w->Stop();
    w->Release();
    delete sys;
}

TEST_CASE("AudioFactory: registered backends are ordered by preference", "[Audio][factory][registry]")
{
    const auto backends = AudioFactory::EnumerateRegistered();
    REQUIRE(backends.size() >= 3);

    CHECK(std::string(backends[0].codeName) == "openal");
    CHECK(backends[0].priority == 1);
    CHECK(std::string(backends[1].codeName) == "dummy");
    CHECK(backends[1].priority == 0);
    CHECK(std::string(backends[2].codeName) == "text");
    CHECK(backends[2].priority == -1);
}

TEST_CASE("AudioFactory: unknown backend is reported", "[Audio][factory][registry]")
{
    const AudioCreateResult created = AudioFactory::Create("missing-backend", false);
    CHECK(created.system == nullptr);
    CHECK(created.status == AudioCreateStatus::UnknownBackend);
}

TEST_CASE("AudioFactory: auto selection skips unavailable backend", "[Audio][factory][registry]")
{
    static const bool registered = AudioFactory::Register(AudioBackendDescriptor{
        "unavailable-test",
        "Unavailable test backend",
        99,
        nullptr,
        []() { return false; },
    });
    (void)registered;

    const AudioCreateResult autoCreated = AudioFactory::Create("auto", false);
    REQUIRE(autoCreated.system != nullptr);
    CHECK(std::string(autoCreated.backend.codeName) != "unavailable-test");
    delete autoCreated.system;

    const AudioCreateResult explicitCreated = AudioFactory::Create("unavailable-test", false);
    CHECK(explicitCreated.system == nullptr);
    CHECK(explicitCreated.status == AudioCreateStatus::Unavailable);
}

TEST_CASE("SoundSystemCapture: full lifecycle", "[Audio][capture]")
{
    SoundSystemCapture sys;
    auto* w = dynamic_cast<WaveCapture*>(sys.CreateWave("lifecycle.wav", false));
    REQUIRE(w != nullptr);

    // Create -> Play -> Render -> Stop -> Verify
    w->SetVolume(0.8f);
    w->Play();
    CHECK(!w->IsStopped());

    sys.RenderAll(50);
    REQUIRE(w->GetCapturedSamples().size() == 50);

    w->Stop();
    CHECK(w->IsStopped());
    // Stop must NOT terminate — audio-invariants A-03.  Backends that
    // collapse Stop -> Terminated break the shared lifecycle contract
    // (audio-bug-catalog B-A20).
    CHECK_FALSE(w->IsTerminated());
    CHECK(w->State() == WaveState::StoppedReplayable);

    // No more samples after stop (playback halted)
    sys.RenderAll(50);
    CHECK(w->GetCapturedSamples().size() == 50);
}

TEST_CASE("SoundSystemDummy: volume round-trip", "[Audio][factory]")
{
    SoundSystemDummy sys;
    sys.SetWaveVolume(0.75f);
    CHECK(sys.GetWaveVolume() == 0.75f);
    sys.SetCDVolume(0.5f);
    CHECK(sys.GetCDVolume() == 0.5f);
    sys.SetSpeechVolume(0.25f);
    CHECK(sys.GetSpeechVolume() == 0.25f);
}
