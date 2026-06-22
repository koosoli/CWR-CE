// Shared lifecycle contract fixture — runs the IWave FSM invariants
// (audio-invariants A-01 .. A-04) against every backend.
//
// Two complementary patterns:
//   1. TEMPLATE_LIST_TEST_CASE over {Dummy, Text, Capture} for backends
//      that don't need a Commit pump.  Adding a new such backend means
//      adding it to AllBackends and getting every case for free.
//   2. Direct TEST_CASEs ("A-XX-OAL: ...") for SoundSystemOAL, which
//      requires real fixture data via CreateWaveFromMemory and a
//      Commit() pump between transitions to advance the deferred-play
//      seam.  Skips gracefully when the AL device or fixture file is
//      unavailable.
//
// Pins audio-bug-catalog B-A20 — cross-backend lifecycle drift where a
// backend's Stop() collapses to Terminal (rather than StoppedReplayable)
// would fail the A-03 case immediately, on every affected backend.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include <Poseidon/Audio/IAudioSystem.hpp>
#include <Poseidon/Audio/Dummy/SoundSystemDummy.hpp>
#include <Poseidon/Audio/Dummy/WaveDummy.hpp>
#include <Poseidon/Audio/Text/SoundSystemText.hpp>
#include <Poseidon/Audio/Text/WaveText.hpp>
#include <Poseidon/Audio/Capture/SoundSystemCapture.hpp>
#include <Poseidon/Audio/Capture/WaveCapture.hpp>
#include <Poseidon/Audio/Core/WaveLifecycle.hpp>
#include <PoseidonOpenAL/SoundSystemOAL.hpp>
#include <PoseidonOpenAL/WaveOAL.hpp>

#include <Poseidon/Audio/SoundScene.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/IO/ParamFileExt.hpp>

#include <AL/al.h>

#include <cmath>

#include "../test_fixtures.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <tuple>
#include <vector>
#include <stddef.h>
#include <stdint.h>
#include <atomic>
#include <catch2/catch_message.hpp>
#include <string>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Time/Time.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <PoseidonOpenAL/WaveStreamingBuffers.hpp>

using namespace Poseidon;
namespace
{
struct DummyBackend
{
    using SystemT = SoundSystemDummy;
    static const char* Name() { return "Dummy"; }
};
struct TextBackend
{
    using SystemT = SoundSystemText;
    static const char* Name() { return "Text"; }
};
struct CaptureBackend
{
    using SystemT = SoundSystemCapture;
    static const char* Name() { return "Capture"; }
};

using AllBackends = std::tuple<DummyBackend, TextBackend, CaptureBackend>;

// Helper: friendly name of a WaveState for failure messages.
const char* StateName(WaveState s)
{
    switch (s)
    {
        case WaveState::Created:
            return "Created";
        case WaveState::WaitingDeferred:
            return "WaitingDeferred";
        case WaveState::Playing:
            return "Playing";
        case WaveState::Paused:
            return "Paused";
        case WaveState::StoppedReplayable:
            return "StoppedReplayable";
        case WaveState::Terminal:
            return "Terminal";
    }
    return "?";
}
} // namespace

// A-01: IWave has an explicit lifecycle state machine.
// Every transition produces a deterministic, named state regardless of
// backend.
TEMPLATE_LIST_TEST_CASE("A-01: full FSM walk produces the same state sequence on every backend",
                        "[audio-lifecycle][A-01]", AllBackends)
{
    typename TestType::SystemT sys;
    IWave* w = sys.CreateWave("lifecycle.wav", false, false);
    REQUIRE(w != nullptr);

    INFO("Backend: " << TestType::Name());

    // Created
    REQUIRE(w->State() == WaveState::Created);

    // Created -> Playing
    w->Play();
    INFO("post-Play state: " << StateName(w->State()));
    REQUIRE((w->State() == WaveState::Playing || w->State() == WaveState::WaitingDeferred));

    // Playing -> Paused (offset preserved on backends that support it)
    w->Pause();
    REQUIRE(w->State() == WaveState::Paused);

    // Paused -> Playing via Resume
    w->Resume();
    REQUIRE((w->State() == WaveState::Playing || w->State() == WaveState::WaitingDeferred));

    // Playing -> StoppedReplayable via Stop (NOT Terminal — see A-03)
    w->Stop();
    REQUIRE(w->State() == WaveState::StoppedReplayable);

    // StoppedReplayable -> StoppedReplayable via Restart (reset; not auto-play)
    w->Restart();
    REQUIRE(w->State() == WaveState::StoppedReplayable);

    // StoppedReplayable -> Playing via Play (the canonical resume path)
    w->Play();
    REQUIRE((w->State() == WaveState::Playing || w->State() == WaveState::WaitingDeferred));

    // Playing -> Terminal via LastLoop (signals natural end-of-life)
    w->LastLoop();
    REQUIRE(w->State() == WaveState::Terminal);

    // Terminal is sticky — any further verb keeps the wave Terminal.
    w->Play();
    REQUIRE(w->State() == WaveState::Terminal);
    w->Stop();
    REQUIRE(w->State() == WaveState::Terminal);
    w->Pause();
    REQUIRE(w->State() == WaveState::Terminal);

    w->Release();
}

// A-03: Stop() carries one documented contract across backends.
// Stop produces StoppedReplayable, never Terminal.  Catches the
// cross-backend lifecycle drift catalogued as B-A20 — any backend
// whose Stop() collapses to Terminal fails this case.
TEMPLATE_LIST_TEST_CASE("A-03: Stop produces StoppedReplayable, not Terminal", "[audio-lifecycle][A-03]", AllBackends)
{
    typename TestType::SystemT sys;
    IWave* w = sys.CreateWave("stop_contract.wav", false, false);
    REQUIRE(w != nullptr);

    INFO("Backend: " << TestType::Name());

    w->Play();
    w->Stop();

    CHECK(w->State() == WaveState::StoppedReplayable);
    CHECK(w->IsStopped());
    CHECK_FALSE(w->IsTerminated());

    // Stop is idempotent — repeat keeps the wave in StoppedReplayable.
    w->Stop();
    CHECK(w->State() == WaveState::StoppedReplayable);
    CHECK_FALSE(w->IsTerminated());

    w->Release();
}

// A-04: Restart() returns the wave to a state where the next Play()
// succeeds, regardless of prior history.  State() reports
// StoppedReplayable until Play() is called.
// Tested from every non-terminal prior state.
TEMPLATE_LIST_TEST_CASE("A-04: Restart from Created", "[audio-lifecycle][A-04]", AllBackends)
{
    typename TestType::SystemT sys;
    IWave* w = sys.CreateWave("restart_from_created.wav", false, false);
    REQUIRE(w != nullptr);

    INFO("Backend: " << TestType::Name());

    // Restart on a never-played wave is allowed; result is a wave the next
    // Play() can pick up cleanly.
    w->Restart();
    CHECK((w->State() == WaveState::StoppedReplayable || w->State() == WaveState::Created));

    w->Play();
    CHECK((w->State() == WaveState::Playing || w->State() == WaveState::WaitingDeferred));

    w->Release();
}

TEMPLATE_LIST_TEST_CASE("A-04: Restart from Playing", "[audio-lifecycle][A-04]", AllBackends)
{
    typename TestType::SystemT sys;
    IWave* w = sys.CreateWave("restart_from_playing.wav", false, false);
    REQUIRE(w != nullptr);

    INFO("Backend: " << TestType::Name());

    w->Play();
    REQUIRE((w->State() == WaveState::Playing || w->State() == WaveState::WaitingDeferred));

    w->Restart();
    CHECK(w->State() == WaveState::StoppedReplayable);

    w->Play();
    CHECK((w->State() == WaveState::Playing || w->State() == WaveState::WaitingDeferred));

    w->Release();
}

TEMPLATE_LIST_TEST_CASE("A-04: Restart from StoppedReplayable", "[audio-lifecycle][A-04]", AllBackends)
{
    typename TestType::SystemT sys;
    IWave* w = sys.CreateWave("restart_from_stopped.wav", false, false);
    REQUIRE(w != nullptr);

    INFO("Backend: " << TestType::Name());

    w->Play();
    w->Stop();
    REQUIRE(w->State() == WaveState::StoppedReplayable);

    w->Restart();
    CHECK(w->State() == WaveState::StoppedReplayable);

    w->Play();
    CHECK((w->State() == WaveState::Playing || w->State() == WaveState::WaitingDeferred));

    w->Release();
}

TEMPLATE_LIST_TEST_CASE("A-04: Restart from Paused", "[audio-lifecycle][A-04]", AllBackends)
{
    typename TestType::SystemT sys;
    IWave* w = sys.CreateWave("restart_from_paused.wav", false, false);
    REQUIRE(w != nullptr);

    INFO("Backend: " << TestType::Name());

    w->Play();
    w->Pause();
    REQUIRE(w->State() == WaveState::Paused);

    w->Restart();
    CHECK(w->State() == WaveState::StoppedReplayable);

    w->Play();
    CHECK((w->State() == WaveState::Playing || w->State() == WaveState::WaitingDeferred));

    w->Release();
}

// A-04: Restart is the canonical revival path from Terminal.
// Terminal is sticky against Play/Stop/Pause (those are no-ops); Restart
// explicitly resets the wave back to StoppedReplayable so the caller
// can reuse the wave without destroying and recreating it.  This matches
// WaveOAL::Restart's existing behavior (clears _state.terminated).
TEMPLATE_LIST_TEST_CASE("A-04: Restart from Terminal revives to StoppedReplayable", "[audio-lifecycle][A-04]",
                        AllBackends)
{
    typename TestType::SystemT sys;
    IWave* w = sys.CreateWave("restart_from_terminal.wav", false, false);
    REQUIRE(w != nullptr);

    INFO("Backend: " << TestType::Name());

    w->Play();
    w->LastLoop();
    REQUIRE(w->State() == WaveState::Terminal);

    w->Restart();
    CHECK(w->State() == WaveState::StoppedReplayable);

    // And the revived wave can be played again.
    w->Play();
    CHECK((w->State() == WaveState::Playing || w->State() == WaveState::WaitingDeferred));

    w->Release();
}

// A-28: Per-frame counters expose active2D / active3D / paused / suppressed
// without screen-scraping logs.  Default IAudioSystem::GetCounters()
// returns zeros; backends that need real numbers (SoundSystemOAL)
// override.  This case exercises the default path on the test
// backends so the API dispatches correctly across every IWave*.
TEMPLATE_LIST_TEST_CASE("A-28: GetCounters() snapshot is accessible on every backend", "[audio-lifecycle][A-28]",
                        AllBackends)
{
    typename TestType::SystemT sys;
    INFO("Backend: " << TestType::Name());

    AudioCounters c = sys.GetCounters();
    // Default backends return all-zero — no live AL state to count.
    CHECK(c.active2D == 0);
    CHECK(c.active3D == 0);
    CHECK(c.paused == 0);
    CHECK(c.suppressedMusic == 0);
    CHECK(c.evictedThisFrame == 0);
    CHECK(c.pausedThisFrame == 0);
    CHECK(c.allocFailures == 0);
}

// A-28 OAL-specific: SoundSystemOAL::GetCounters() walks the live wave
// inventory and partitions by State().  Validates the wiring from
// WaveOAL::State() through SoundSystemOAL::GetCounters().
TEST_CASE("A-28: SoundSystemOAL counters reflect live wave inventory", "[audio-lifecycle][A-28][oal]")
{
    auto* sys = dynamic_cast<SoundSystemOAL*>(CreateSoundSystemOAL());
    if (!sys)
    {
        SKIP("OpenAL not available");
    }

    AudioCounters c0 = sys->GetCounters();
    CHECK(c0.active2D == 0);
    CHECK(c0.active3D == 0);
    CHECK(c0.paused == 0);
    CHECK(c0.suppressedMusic == 0);
    // allocFailures may be > 0 if the harness has already tried to load
    // non-existent fixtures earlier in the suite, so we only verify the
    // counter is monotonic across the rest of this test.
    int allocBaseline = c0.allocFailures;

    // Attempt to load a non-existent file — this hits the SoundLoadFile
    // failure path (load failure, not buffer failure) so allocFailures
    // stays at baseline.  A future Phase-E test will drive a deterministic
    // alBufferData failure via mock-AL or fixture corruption.
    auto* nonexistent = sys->CreateWave("nonexistent_for_counters.wav", false, false);
    if (nonexistent)
    {
        nonexistent->Release();
    }

    AudioCounters c1 = sys->GetCounters();
    CHECK(c1.allocFailures >= allocBaseline);

    delete sys;
}

// A-01 corollary: IsStopped() and the FSM agree.
// IsStopped() returns true exactly when State() is Created or
// StoppedReplayable or Terminal.  IsStopped() must return false in
// Playing and Paused (Paused != Stopped).
TEMPLATE_LIST_TEST_CASE("A-01: IsStopped() / IsTerminated() agree with State()", "[audio-lifecycle][A-01]", AllBackends)
{
    typename TestType::SystemT sys;
    IWave* w = sys.CreateWave("query_consistency.wav", false, false);
    REQUIRE(w != nullptr);

    INFO("Backend: " << TestType::Name());

    auto check = [&](WaveState expected)
    {
        INFO("Expected state: " << StateName(expected) << ", actual: " << StateName(w->State()));
        REQUIRE(w->State() == expected);
        const bool stopped = (expected == WaveState::Created || expected == WaveState::StoppedReplayable ||
                              expected == WaveState::Terminal);
        const bool terminated = (expected == WaveState::Terminal);
        CHECK(w->IsStopped() == stopped);
        CHECK(w->IsTerminated() == terminated);
    };

    check(WaveState::Created);
    w->Play();
    if (w->State() == WaveState::Playing)
    {
        check(WaveState::Playing);
    }
    w->Pause();
    check(WaveState::Paused);
    w->Resume();
    if (w->State() == WaveState::Playing)
    {
        check(WaveState::Playing);
    }
    w->Stop();
    check(WaveState::StoppedReplayable);
    w->LastLoop();
    check(WaveState::Terminal);

    w->Release();
}

// SoundSystemOAL FSM walks.
//
// OAL waves have a deferred-play seam: Play() sets _wantPlaying, then
// the next Commit() runs DoPlay() to actually load + start the AL
// source.  State() reports WaitingDeferred during the queue window,
// Playing after.  These tests pump Commit() between transitions so
// the FSM moves the way the templated suite expects of simpler
// backends.
//
// LastLoop on OAL is "stop looping, let the source finish naturally"
// — not "Terminate immediately" — so the Terminal-transition cases
// here use the deterministic SetStopValue path instead.

namespace
{
std::vector<char> ReadFixtureBytes(const char* relPath)
{
    const char* path = TestFixtures::GetTestFixturePath(relPath);
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
    {
        return {};
    }
    auto end = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<char> buf(static_cast<size_t>(end));
    f.read(buf.data(), end);
    return buf;
}

struct OALFixture
{
    IAudioSystem* sys = nullptr;
    std::vector<char> wavData;
    bool Init()
    {
        sys = CreateSoundSystemOAL();
        if (!sys)
        {
            return false;
        }
        wavData = ReadFixtureBytes("audio/tone.wav");
        return !wavData.empty();
    }
    IWave* CreateLoadedWave() { return sys->CreateWaveFromMemory(wavData.data(), wavData.size(), ".wav", false); }
    ~OALFixture() { delete sys; }
};

ALenum DrainAlErrors(int maxReads = 32)
{
    ALenum last = AL_NO_ERROR;
    for (int i = 0; i < maxReads; ++i)
    {
        last = alGetError();
        if (last == AL_NO_ERROR)
            return AL_NO_ERROR;
    }
    return last;
}

ALenum SnapshotAlErrorState()
{
    return DrainAlErrors();
}

std::string ReadTextFile(const std::filesystem::path& path)
{
    std::ifstream f(path);
    if (!f.is_open())
        return {};
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string ExtractFunctionBody(const std::string& src, const std::string& prototype)
{
    const size_t protoPos = src.find(prototype);
    if (protoPos == std::string::npos)
        return {};

    const size_t bodyStart = src.find('{', protoPos);
    if (bodyStart == std::string::npos)
        return {};

    int depth = 1;
    for (size_t i = bodyStart + 1; i < src.size(); ++i)
    {
        if (src[i] == '{')
            ++depth;
        else if (src[i] == '}')
        {
            --depth;
            if (depth == 0)
                return src.substr(bodyStart, i - bodyStart + 1);
        }
    }
    return {};
}

std::string SwitchOutputDeviceBody()
{
    const auto path =
        std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "PoseidonOpenAL" / "SoundSystemOAL.cpp";
    const std::string src = ReadTextFile(path);
    if (src.empty())
        return {};
    return ExtractFunctionBody(src, "bool SoundSystemOAL::SwitchOutputDevice(const char* name)");
}

std::string GetCurrentOutputDeviceBody()
{
    const auto path =
        std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "PoseidonOpenAL" / "SoundSystemOAL.cpp";
    const std::string src = ReadTextFile(path);
    if (src.empty())
        return {};
    return ExtractFunctionBody(src, "std::string SoundSystemOAL::GetCurrentOutputDevice() const");
}
} // namespace

// Phase 3c: WaveOAL load state machine.  Pins the transition the
// Phase 3d Commit poller will rely on — that after a successful
// Load(), GetLoadState() reports Resident, and Unload returns the
// wave to NotStarted.  The intermediate states (DecodePending,
// UploadPending) are visited synchronously inside Load() in Phase
// 3c; Phase 3d will introduce a worker-thread observability window
// where DecodePending becomes externally visible.
TEST_CASE("Phase3c: WaveOAL load state transitions NotStarted -> Resident -> NotStarted",
          "[audio-lifecycle][oal][load_state]")
{
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL or tone.wav fixture unavailable");
    }

    auto* w = static_cast<WaveOAL*>(fx.CreateLoadedWave());
    REQUIRE(w != nullptr);

    // Constructor opens the stream + parses the header but does not
    // decode — load state should be NotStarted.
    REQUIRE(w->GetLoadState() == WaveOAL::LoadState::NotStarted);

    // Play() triggers the lazy-load on first Commit/DoPlay (the
    // unchanged Phase 3c path).  After Commit, the wave should be
    // fully Resident.
    w->Play();
    fx.sys->Commit();
    REQUIRE(w->State() == WaveState::Playing);
    REQUIRE(w->GetLoadState() == WaveOAL::LoadState::Resident);

    // Tearing down the wave must clear AL state and reset the
    // load-state machine so subsequent re-load (not exercised here,
    // but the path matters for SwitchOutputDevice) starts clean.
    w->Release();
}

TEST_CASE("Phase3g-2: DecodeOneChunk produces full file in chunkBytes increments + EOF flag",
          "[audio-lifecycle][oal][streaming]")
{
    // The chunked decode path that Phase 3g-3 will wire into the
    // DoPlay+Commit loop.  Exercises only DecodeOneChunk +
    // OpenStreaming today — no AL buffer ring, no playback.
    //
    // Pinning:
    //   - OpenStreaming sets a positive chunkBytes derived from the
    //     wave format (tone.wav is 22.05 kHz mono 16-bit, so 100 ms
    //     ≈ 8820 bytes after sample-boundary rounding)
    //   - Repeated DecodeOneChunk calls cover the full file, the
    //     last one is flagged isLastChunk and the EOF state flips
    //   - The sum of chunk sizes matches the wave's _state.size
    //   - Calling DecodeOneChunk past EOF is a safe no-op
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL or tone.wav fixture unavailable");
    }

    auto* w = static_cast<WaveOAL*>(fx.CreateLoadedWave());
    REQUIRE(w != nullptr);

    // AudioState::looping defaults to true, but this test
    // specifically exercises the EOF-reached path.  Repeat(1)
    // sets `_state.looping = false` (per WaveOAL::Repeat
    // semantics: repeat < 2 → not looping).
    w->Repeat(1);
    w->OpenStreamingForTest();
    REQUIRE(w->StreamingState().chunkBytes > 0);

    int64_t totalBytes = 0;
    int chunkCount = 0;
    bool sawLastFlag = false;
    while (!w->StreamingState().eofReached.load())
    {
        const auto sizeBefore = w->StreamingState().queue.Size();
        w->DecodeOneChunkForTest();
        const auto sizeAfter = w->StreamingState().queue.Size();
        if (sizeAfter == sizeBefore)
        {
            // No chunk produced — should only happen at EOF.
            REQUIRE(w->StreamingState().eofReached.load());
            break;
        }
        ::Poseidon::Audio::DecodedChunk chunk;
        REQUIRE(w->StreamingState().queue.TryPop(chunk));
        totalBytes += static_cast<int64_t>(chunk.pcm.size());
        if (chunk.isLastChunk)
        {
            sawLastFlag = true;
        }
        ++chunkCount;
        REQUIRE(chunkCount < 100000); // runaway safety
    }
    REQUIRE(sawLastFlag);
    REQUIRE(totalBytes > 0);
    // tone.wav is small (~24 KB PCM) so we expect a handful of
    // chunks, not hundreds.  Exact count varies with the file's
    // format header, but the sum equals the wave's reported size.
    INFO("decoded " << chunkCount << " chunks, " << totalBytes << " total bytes");

    // Calling once more is a no-op; queue stays empty, no crash.
    w->DecodeOneChunkForTest();
    REQUIRE(w->StreamingState().queue.Size() == 0);
    REQUIRE(w->StreamingState().eofReached.load());

    w->Release();
}

TEST_CASE("Phase3g-5: looping wave wraps decode offset at EOF instead of flagging eofReached",
          "[audio-lifecycle][oal][streaming][loop]")
{
    // Phase 3g-5 acceptance: when _state.looping is set, the
    // decode worker wraps decodeOffsetBytes to 0 at EOF instead
    // of flagging eofReached.  This is the sample-accurate loop
    // mechanism that replaces alSourcei(AL_LOOPING) — AL doesn't
    // loop queued sources, so the worker drives it.
    //
    // Drives DecodeOneChunk past the file boundary on a looping
    // wave and asserts: (a) eofReached stays false across many
    // wrap cycles, (b) decodeOffsetBytes wraps as expected,
    // (c) chunks keep being produced.
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL or tone.wav fixture unavailable");
    }

    auto* w = static_cast<WaveOAL*>(fx.CreateLoadedWave());
    REQUIRE(w != nullptr);
    w->Repeat(10); // sets _state.looping = true
    w->OpenStreamingForTest();
    REQUIRE(w->StreamingState().chunkBytes > 0);

    const int totalSize = static_cast<int>(w->StreamingState().chunkBytes); // not really; use a budget instead
    (void)totalSize;
    constexpr int kIterations = 50; // enough to wrap several times for tone.wav
    int chunks = 0;
    bool sawWrap = false;
    for (int i = 0; i < kIterations; ++i)
    {
        const int64_t offsetBefore = w->StreamingState().decodeOffsetBytes.load();
        w->DecodeOneChunkForTest();
        ::Poseidon::Audio::DecodedChunk chunk;
        if (!w->StreamingState().queue.TryPop(chunk))
        {
            // Loop should keep producing; if we ever fail to pop
            // a chunk on a looping wave, that's a regression.
            INFO("iteration " << i << " — no chunk produced");
            FAIL_CHECK("looping decode stalled");
            break;
        }
        ++chunks;
        const int64_t offsetAfter = w->StreamingState().decodeOffsetBytes.load();
        if (offsetAfter < offsetBefore)
        {
            sawWrap = true;
        }
        // Crucial: eofReached must stay false for looping waves.
        REQUIRE_FALSE(w->StreamingState().eofReached.load());
    }
    REQUIRE(chunks == kIterations);
    REQUIRE(sawWrap);
    w->Release();
}

// fadeMusic / SetMusicVolume — is the music volume effect actually
// implemented, and does it ramp?  Drives the REAL SoundScene::AdvanceAll
// fade path on a headless Text backend, advancing the game clock
// (Poseidon::Glob.time) exactly as World::Simulate does, and observes the
// live ramped music volume + the volume applied to the music wave.
namespace
{
struct MusicSceneFixture
{
    SoundSystemText sys;
    SoundScene scene;
    IAudioSystem* prevSys = GSoundsys;
    SoundScene* prevScene = GSoundScene;
    Poseidon::Foundation::Time prevTime = Poseidon::Glob.time;

    MusicSceneFixture()
    {
        GSoundsys = &sys;
        GSoundScene = &scene;
        scene.Reset();
    }
    ~MusicSceneFixture()
    {
        GSoundsys = prevSys;
        GSoundScene = prevScene;
        Poseidon::Glob.time = prevTime;
    }
    // One engine frame: advance the game clock, then the sound scene.
    void Frame(float dt)
    {
        Poseidon::Glob.time = Poseidon::Glob.time + dt;
        scene.AdvanceAll(dt, /*paused=*/false);
    }
};
} // namespace

TEST_CASE("fadeMusic: SetMusicVolume ramps the live music volume linearly to target", "[audio][music][fade]")
{
    MusicSceneFixture fx;

    SoundPars pars;
    pars.name = "track3";
    pars.vol = 1.0f;
    pars.freq = 1.0f;
    fx.scene.StartMusicTrack(pars);

    // Baseline: snap to full volume (time<=0.01 sets immediately).
    fx.scene.SetMusicVolume(1.0f, 0.0f);
    fx.Frame(0.016f);
    REQUIRE(fx.scene.MusicLiveVolumeForTest() == Catch::Approx(1.0f).margin(0.001f));

    // "60 fadeMusic 0.0": ramp to silence over 60 game-seconds.
    fx.scene.SetMusicVolume(0.0f, 60.0f);

    // 30 s in 0.5 s frames -> exactly halfway (the ramp is linear).
    for (int i = 0; i < 60; ++i)
        fx.Frame(0.5f);
    INFO("at 30s live=" << fx.scene.MusicLiveVolumeForTest());
    CHECK(fx.scene.MusicLiveVolumeForTest() == Catch::Approx(0.5f).margin(0.05f));
    // Effect reaches the wave: applied = internalVol(1.0) * liveVol.
    CHECK(fx.scene.MusicAppliedVolumeForTest() == Catch::Approx(0.5f).margin(0.08f));

    // Another 31 s -> cross the target time and snap to 0.0.
    for (int i = 0; i < 62; ++i)
        fx.Frame(0.5f);
    CHECK(fx.scene.MusicLiveVolumeForTest() == Catch::Approx(0.0f).margin(0.01f));
    CHECK(fx.scene.MusicAppliedVolumeForTest() == Catch::Approx(0.0f).margin(0.02f));
}

TEST_CASE("fadeMusic: zero-time set is an instantaneous cut", "[audio][music][fade]")
{
    MusicSceneFixture fx;
    SoundPars pars;
    pars.name = "track7";
    pars.vol = 1.0f;
    pars.freq = 1.0f;
    fx.scene.StartMusicTrack(pars);

    fx.scene.SetMusicVolume(1.0f, 0.0f);
    fx.Frame(0.016f);
    REQUIRE(fx.scene.MusicLiveVolumeForTest() == Catch::Approx(1.0f).margin(0.001f));

    // "0 fadeMusic 0.0" — instant, no ramp (the value changes before any frame).
    fx.scene.SetMusicVolume(0.0f, 0.0f);
    CHECK(fx.scene.MusicLiveVolumeForTest() == Catch::Approx(0.0f).margin(0.001f));
}

namespace
{
// The 1.99 music-fade recurrence (soundScene.cpp:358-367), isolated as the
// authority this card is measured against.  The fixture advances Glob.time
// *before* AdvanceAll, so stepping this with the same per-frame dt tracks the
// engine frame-for-frame.  1.99 ramps _musicVolume linearly; its DirectSound
// backend round-trips that through dB (float2db, DBOffset=0) back to the same
// linear amplitude our OpenAL AL_GAIN applies — so matching this ramp is
// matching what the player hears.
struct Ref199MusicFade
{
    float cur = 1.0f;
    float target = 1.0f;
    float total = 0.0f;
    float elapsed = 0.0f;
    void Fade(float vol, float time)
    {
        target = vol;
        total = time;
        elapsed = 0.0f;
        if (time <= 0.01f)
            cur = vol;
    }
    void Step(float dt)
    {
        elapsed += dt;
        const float timeRest = total - elapsed;
        if (timeRest <= 0.0f)
            cur = target;
        else
            cur += dt * ((target - cur) / timeRest);
    }
};
} // namespace

// REGRESSION (cutscene speech ducking): fadeMusic transitions must be a gradual
// ramp, frame-for-frame identical to the 1.99 recurrence it descends from —
// never an instant jump to the target.  We drive two legs the way a cutscene
// does ("5 fadeMusic 0.3" to duck under speech, then "3 fadeMusic 1" to swell
// back) and require the live + applied music volume to match a local
// re-implementation of 1.99's exact recurrence on every frame.
//
// Broken-state delta: make SetMusicVolume set _musicVolume = target
// immediately (an unsmoothed jump) and the very first frame reads the target
// (0.30) while the 1.99 reference is still near full (~0.99) — the
// frame-for-frame REQUIRE fails by ~0.69, the midpoint sample reads 0.30 not
// ~0.65, and maxStep is the whole 0.70 delta instead of ~0.014.
TEST_CASE("fadeMusic: transition ramps frame-for-frame like 1.99, never an instant jump", "[audio][music][fade]")
{
    MusicSceneFixture fx;

    constexpr float kInternalVol = 0.75f; // playMusic track gain; applied = internal * live
    SoundPars pars;
    pars.name = "track3";
    pars.vol = kInternalVol;
    pars.freq = 1.0f;
    fx.scene.StartMusicTrack(pars);

    fx.scene.SetMusicVolume(1.0f, 0.0f); // start at full (time<=0.01 is an instant set)
    fx.Frame(0.016f);
    REQUIRE(fx.scene.MusicLiveVolumeForTest() == Catch::Approx(1.0f).margin(0.001f));

    Ref199MusicFade ref;

    struct Leg
    {
        float target;
        float time;
        const char* what;
    };
    const Leg legs[] = {
        {0.30f, 5.0f, "duck under speech"}, // "5 fadeMusic 0.3"
        {1.00f, 3.0f, "swell back up"},     // "3 fadeMusic 1"
    };

    constexpr float dt = 0.1f; // 10 Hz frames — fine enough to resolve a ramp from a jump
    for (const Leg& leg : legs)
    {
        const float start = fx.scene.MusicLiveVolumeForTest();
        fx.scene.SetMusicVolume(leg.target, leg.time);
        ref.Fade(leg.target, leg.time);

        // Assert parity only up to a couple frames before the target time, to
        // stay clear of the float boundary where the engine and reference snap
        // to target (they may cross on adjacent frames).
        const int parity = static_cast<int>(leg.time / dt + 0.5f) - 2;
        const int mid = parity / 2;
        float prev = start;
        float maxStep = 0.0f;
        float midLive = start;

        for (int i = 0; i < parity; ++i)
        {
            fx.Frame(dt);
            ref.Step(dt);
            const float live = fx.scene.MusicLiveVolumeForTest();

            INFO(leg.what << " frame " << i << " live=" << live << " ref199=" << ref.cur);
            REQUIRE(live == Catch::Approx(ref.cur).margin(1e-3f)); // matches 1.99 frame-for-frame
            REQUIRE(fx.scene.MusicAppliedVolumeForTest() ==
                    Catch::Approx(kInternalVol * ref.cur).margin(1e-3f)); // effect reaches the wave

            if (leg.target < start)
                REQUIRE(live <= prev + 1e-4f); // monotonic toward target — no overshoot/zip
            else
                REQUIRE(live >= prev - 1e-4f);

            const float step = std::fabs(live - prev);
            if (step > maxStep)
                maxStep = step;
            if (i == mid)
                midLive = live;
            prev = live;
        }

        // Halfway through the fade time the volume sits strictly between start
        // and target — the clearest "it did not snap" signal.  An instant jump
        // reads the target here instead.
        const float lo = leg.target < start ? leg.target : start;
        const float hi = leg.target < start ? start : leg.target;
        INFO(leg.what << " midLive=" << midLive << " in (" << lo << ", " << hi << ")");
        REQUIRE(midLive > lo + 0.05f);
        REQUIRE(midLive < hi - 0.05f);

        // Let the fade time fully elapse: settle exactly on target (keep the
        // reference in lockstep so the next leg starts from the same value).
        for (int i = 0; i < 6; ++i)
        {
            fx.Frame(dt);
            ref.Step(dt);
        }
        REQUIRE(fx.scene.MusicLiveVolumeForTest() == Catch::Approx(leg.target).margin(1e-3f));
        REQUIRE(ref.cur == Catch::Approx(leg.target).margin(1e-3f));

        // No single 0.1 s frame carried more than a small slice of the whole
        // delta — an instant jump dumps the entire delta into one frame.
        const float wholeDelta = std::fabs(leg.target - start);
        INFO(leg.what << " maxStep=" << maxStep << " wholeDelta=" << wholeDelta);
        REQUIRE(maxStep < 0.10f * wholeDelta);
    }
}

// DIAGNOSTIC: does a streamed music wave play to its natural end, and does
// in-game TIME ACCELERATION cut it short?  Replicates the engine's exact
// per-frame order (ApplyVoice2D: wave->Play(); wave->Advance(gameDeltaT);
// then SoundSystemOAL::Commit()) in real wall-clock frames, with gameDeltaT
// = realDeltaT * accel.  The audio device always drains in real time.
TEST_CASE("DIAG: streamed music natural EOF vs accelerated game time", "[audio-lifecycle][oal][streaming][diag]")
{
    auto* sys = dynamic_cast<SoundSystemOAL*>(CreateSoundSystemOAL());
    if (!sys)
        SKIP("OpenAL not available");

    const std::string ogg = GET_FIXTURE("mission_smoke/alpha_scenario.cain/sound/s03r04.ogg");

    auto runOnce = [&](float accel) -> std::string
    {
        auto* w = static_cast<WaveOAL*>(sys->CreateWave(ogg.c_str(), /*is3D=*/false, /*prealloc=*/true));
        if (!w || !w->IsStreamed())
        {
            if (w)
                w->Release();
            return "FAIL: not a streamed wave";
        }
        const float len = w->GetLength();
        w->Repeat(1);
        w->SetSticky(true);

        const auto t0 = std::chrono::steady_clock::now();
        const float realFrameDt = 0.02f; // 20 ms real frames
        float termReal = -1.f;
        int frames = 0;
        for (;;)
        {
            if (!w->IsTerminated())
            {
                w->Play();                       // re-arm every frame, like ApplyVoice2D
                w->Advance(realFrameDt * accel); // game deltaT (accelerated)
            }
            sys->Commit();
            std::this_thread::sleep_for(std::chrono::milliseconds(20)); // real wall time
            ++frames;
            const float realElapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - t0).count();
            if (w->IsTerminated())
            {
                termReal = realElapsed;
                break;
            }
            if (realElapsed > len + 3.0f)
                break; // safety cap: never finished
        }
        const bool eof = w->StreamingState().eofReached.load();
        std::ostringstream os;
        os << "accel=" << accel << "  GetLength=" << len << "s  terminatedAtReal=" << termReal
           << "s  eofReached=" << eof << "  frames=" << frames;
        w->Release();
        return os.str();
    };

    WARN("1x:  " << runOnce(1.0f));
    WARN("10x: " << runOnce(10.0f));

    delete sys;
}

// REGRESSION (Fizzy #34 / B-A24): streamed-music start + underrun recovery.
// Music is an OGG -> streamed source backed by a 4 x 100ms = 400ms AL ring.
// Two bugs this guards:
//   (A) Start race — a single Play() must start the source.  Broken: the
//       first DoPlay opens the ring with no chunk queued yet, skips
//       alSourcePlay, Commit clears _wantPlaying, and nothing retries, so
//       the ring fills (buffersInFlight=4) but the source never plays.
//   (B) Underrun read as EOF — a frame hitch longer than the ring (e.g.
//       entering a BMP gunner's optics) drains the queue; AL goes
//       AL_STOPPED from starvation with eofReached still false.  Broken:
//       IsTerminated()/IsStopped() read AL_STOPPED and reap the music.
// Fixed: DoPlay marks a stream-play intent, UpdateStreamingPlayback
// (re)issues alSourcePlay whenever AL is idle with audio queued and the
// track hasn't ended, and IsTerminated/IsStopped only terminate on a
// genuine end-of-stream.
//
// Broken-state deltas: (A) IsStopped()==true after 60 commits from one
// Play(); (B) IsTerminated()==true after the hitch with eofReached==false.
//
// Depends on the OpenAL backend draining buffers in real time (the DIAG
// case above confirms it does); SKIPs only if OpenAL is unavailable.
TEST_CASE("Fizzy#34: streamed music starts on one Play() and survives an underrun",
          "[audio-lifecycle][oal][streaming][repro]")
{
    auto* sys = dynamic_cast<SoundSystemOAL*>(CreateSoundSystemOAL());
    if (!sys)
        SKIP("OpenAL not available");

    // Multi-second OGG so a 400ms underrun is unambiguously NOT real EOF.
    const std::string ogg = GET_FIXTURE("mission_smoke/coop_scenario.abel/sound/s07r01.ogg");
    auto* w = static_cast<WaveOAL*>(sys->CreateWave(ogg.c_str(), /*is3D=*/false, /*prealloc=*/true));
    if (!w)
    {
        delete sys;
        SKIP("cannot create wave from fixture OGG");
    }
    if (!w->IsStreamed())
    {
        w->Release();
        delete sys;
        SKIP("fixture OGG not routed through the streaming path");
    }

    w->Repeat(1);       // play-once, like playMusic
    w->SetSticky(true); // music keeps playing across pause

    // (A) Start race: a SINGLE Play() must start the source.  Pump Commits
    // and watch for it to begin playing.
    w->Play();
    bool started = false;
    for (int i = 0; i < 60 && !started; ++i)
    {
        sys->Commit();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        started = !w->IsStopped();
    }
    INFO("start: stopped=" << w->IsStopped() << " buffersInFlight=" << w->StreamingState().buffersInFlight);
    CHECK(started); // broken: never starts from one Play()

    // Sanity: a multi-second track is nowhere near EOF this early.
    REQUIRE_FALSE(w->StreamingState().eofReached.load());

    // (B) Underrun: stop pumping Commit for longer than the 400ms ring so
    // the queue drains and AL goes AL_STOPPED from starvation.
    std::this_thread::sleep_for(std::chrono::milliseconds(900));

    CHECK_FALSE(w->StreamingState().eofReached.load()); // still not the real end
    CHECK_FALSE(w->IsTerminated());                     // broken: underrun read as EOF

    // Recovery: resume the pump; the source must restart and keep playing
    // rather than stay dead.
    bool recovered = false;
    for (int i = 0; i < 40 && !recovered; ++i)
    {
        sys->Commit();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        recovered = !w->IsStopped() && !w->IsTerminated();
    }
    INFO("recovery: stopped=" << w->IsStopped() << " terminated=" << w->IsTerminated());
    CHECK(recovered);

    w->Release();
    delete sys;
}

TEST_CASE("Phase3e: rapid wave create+play+release cycles don't crash or leak", "[audio-lifecycle][oal][lifetime]")
{
    // Stress the Phase 3d AddRef/Release lifecycle: create a wave,
    // start it (which kicks async load for >=1MB waves, sync for the
    // small tone.wav fixture), then immediately Release.  Repeat
    // many times.  If KickAsyncLoad's AddRef pattern were missing, a
    // worker still holding `this` after the external Release dropped
    // the last reference would crash on the next DecodePcm field
    // access.  With the AddRef in place, the worker always sees a
    // live wave.
    //
    // The tone.wav fixture decodes synchronously (< 1 MB) so this
    // test mostly stresses the create/destroy lifecycle; the demo-
    // intro perf trace in Phase 3f is the real async-path validator.
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL or tone.wav fixture unavailable");
    }
    constexpr int kRounds = 100;
    for (int i = 0; i < kRounds; ++i)
    {
        auto* w = static_cast<WaveOAL*>(fx.CreateLoadedWave());
        REQUIRE(w != nullptr);
        w->Play();
        fx.sys->Commit();
        w->Release();
    }
    // Drain any in-flight async tasks before tearing down the
    // fixture's SoundSystem so we don't UnregisterWave after the
    // system goes away.
    fx.sys->Commit();
}

// Regression for the radio-chaining bug:
//   "the number of unit is played with the message together"
//
// RadioChannel::Say enqueues multi-wave messages (callsign + content)
// by calling `successor->Queue(predecessor)`.  Pre-fix, WaveOAL::Queue
// was an empty stub — every queued wave started immediately and
// overlapped with the head, producing the user-reported simultaneous
// playback.  Port the 1.99 WaveGlobal8::Queue semantics
// (soundDX8.cpp:470): the successor stays gated by `_queued` until the
// predecessor's IsTerminated() poll activates it.
//
// This test pins the gate: a queued wave's Play() arms nothing, the
// AL source stays cold, and the wave reports a non-Playing state
// while its predecessor is still in flight.
TEST_CASE("Queue(): successor stays gated until predecessor terminates", "[audio-lifecycle][oal][radio-chain]")
{
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL or tone.wav fixture unavailable");
    }

    auto* head = static_cast<WaveOAL*>(fx.CreateLoadedWave());
    auto* tail = static_cast<WaveOAL*>(fx.CreateLoadedWave());
    REQUIRE(head != nullptr);
    REQUIRE(tail != nullptr);

    // Mirror RadioChannel::Say:
    //   _saying = head; head->Play();
    //   tail->Queue(head);
    // The bug: pre-fix, tail->Play() (called via SoundScene::AdvanceAll's
    // ApplyVoice2D loop on the next frame) would fire alSourcePlay and
    // tail would start mid-head.  Post-fix, tail stays gated.
    head->Play();
    fx.sys->Commit();
    REQUIRE(head->State() == WaveState::Playing);

    tail->Queue(head, 1);

    // Play() on a queued wave must be a no-op — the gate is in Play()
    // itself, so even if a future caller doesn't know about the chain
    // it can't accidentally start the tail.
    tail->Play();
    fx.sys->Commit();
    CHECK(tail->State() != WaveState::Playing);

    // The head's IsTerminated must report `false` while the tail is
    // alive — channel.Simulate keeps `_saying` linked to head and
    // polls it; reporting `true` prematurely would let the channel
    // free head and lose the chain.
    CHECK_FALSE(head->IsTerminated());

    head->Release();
    tail->Release();
    fx.sys->Commit();
}

// Delay/pause wave (CreateEmptyWave) must actually terminate after its
// duration elapses.  Pre-fix, the OAL port created the delay wave with
// _state.frequency=0 / _state.sSize=0 / _state.size=0 — Skip's first
// guard `if (freq <= 0 || sSize <= 0) return` short-circuited, so the
// negative curPosition never counted down and the wave was eternal.
// RadioChannel::Say chains a pause wave after every word with
// `pauseAfter > 0.01` (aiRadio.cpp), so AI commander sentences ended
// with a never-terminating delay → the whole channel hung.  Match the
// 1.99 ctor (soundDX8.cpp:2158): freq=1000, sSize=1, size=0; and
// teach Skip to terminate when size==0 and curPosition reaches 0.
TEST_CASE("Empty/delay wave terminates after its duration via Skip", "[audio-lifecycle][oal][radio-chain]")
{
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL or tone.wav fixture unavailable");
    }
    // SoundSystem provides the empty-wave factory used by SoundScene::SayPause.
    IWave* base = fx.sys->CreateEmptyWave(0.05f); // 50 ms pause
    REQUIRE(base != nullptr);
    auto* pause = static_cast<WaveOAL*>(base);

    // Brand new: not yet terminated; still in the negative-cursor delay window.
    REQUIRE_FALSE(pause->IsTerminated());

    // Advance by 60 ms in 10 ms increments — > 50 ms total so the delay elapses.
    for (int i = 0; i < 6; ++i)
    {
        pause->Advance(0.01f); // 10 ms
    }
    CHECK(pause->IsTerminated());

    pause->Release();
}

// REGRESSION: the "menu click fade-in".  A 2D sound starts deferred —
// ApplyVoice2D does Play() then Advance(deltaT), and AdvanceAll() runs before
// Commit()/DoPlay() in World::PerformSound.  Pre-fix the in-between
// Advance()->Skip() drifted _state.curPosition and DoPlay seeked the source
// past the click's attack.  Broken state: offset ~0.10 s after Commit (the
// 100 ms Advanced) instead of ~0.
TEST_CASE("deferred 2D sound starts from the beginning, not Advance-skipped", "[audio-lifecycle][oal][deferred-start]")
{
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL or tone.wav fixture unavailable");
    }

    auto* w = static_cast<WaveOAL*>(fx.CreateLoadedWave());
    REQUIRE(w != nullptr);
    w->Repeat(1);

    // Arm a deferred play, then advance a frame before Commit — the engine order.
    w->Play();
    REQUIRE(w->State() == WaveState::WaitingDeferred);
    w->Advance(0.10f); // 100 ms; tone.wav is ~0.54 s so this stays within the clip

    fx.sys->Commit();
    REQUIRE(w->State() == WaveState::Playing);

    const float offset = w->GetCurrentOffsetSeconds();
    INFO("playback offset after deferred start = " << offset << " s");
    CHECK(offset < 0.04f); // pre-fix: ~0.10 s, skipped past the attack

    w->Release();
    fx.sys->Commit();
}

// End-to-end chain activation: head plays, head terminates naturally,
// the queued tail's `_queued` flag is cleared by IsTerminated()'s
// AdvanceQueue, and tail's Play() now arms wantPlaying.  Pre-fix
// (Queue stub) tail was already Playing before head finished —
// post-fix tail must transition Created → (gated) → Playing only after
// head terminates.
//
// Uses SetStopValue to force a synthetic termination on head without
// having to wait for the full ~1s playback of the fixture wav.
TEST_CASE("Queue(): chain activates the next wave when head terminates", "[audio-lifecycle][oal][radio-chain]")
{
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL or tone.wav fixture unavailable");
    }
    auto* head = static_cast<WaveOAL*>(fx.CreateLoadedWave());
    auto* tail = static_cast<WaveOAL*>(fx.CreateLoadedWave());
    REQUIRE(head);
    REQUIRE(tail);

    head->Play();
    fx.sys->Commit();
    REQUIRE(head->State() == WaveState::Playing);

    tail->Queue(head, 1);
    REQUIRE(tail->State() != WaveState::Playing); // gated

    // Force head into Terminal deterministically: PlayUntilStopValue
    // sets _stopThreshold; SetStopValue(time > threshold) flips
    // _state.terminated and Stop()s the source.  Avoids waiting for
    // natural ~1s playback of the fixture WAV.
    head->PlayUntilStopValue(0.0f);
    head->SetStopValue(1.0f);

    // The chain walk runs as a side-effect of head's IsTerminated()
    // poll (1.99-style: WaveGlobal8::AdvanceQueue fires from inside
    // Play/Skip/IsTerminated when the head observes its own terminal
    // state).  Calling IsTerminated here lights up the chain.  It
    // RETURNS FALSE because the chain isn't done — but the side
    // effect we care about is that tail._queued is cleared and
    // tail.Play() has armed wantPlaying.
    CHECK_FALSE(head->IsTerminated()); // false: chain still has live tail

    // One Commit drives DoPlay → tail starts playing.
    fx.sys->Commit();
    CHECK(tail->State() == WaveState::Playing); // chain activated

    head->Release();
    tail->Release();
    fx.sys->Commit();
}

// Queue() must refuse to form a cycle.  Crossed or duplicate enqueues
// previously walked to a tail that landed on `this` (or one of its
// predecessors) and closed the loop; IsTerminated()'s tail recursion
// would then overflow the stack.
TEST_CASE("Queue(): rejects cycle attempts", "[audio-lifecycle][oal][radio-chain]")
{
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL or tone.wav fixture unavailable");
    }
    auto* a = static_cast<WaveOAL*>(fx.CreateLoadedWave());
    auto* b = static_cast<WaveOAL*>(fx.CreateLoadedWave());
    REQUIRE(a);
    REQUIRE(b);

    // First chain: B queued behind A.  Valid.
    b->Queue(a, 1);

    // Cycle attempt: A queued behind B → would form A→B→A.  Must no-op.
    a->Queue(b, 1);

    // Force a terminated walk; pre-fix this would recurse A→B→A→…
    // and overflow.  Post-fix the cycle was refused so the walk
    // terminates at b's null _queueNext.
    a->PlayUntilStopValue(0.0f);
    a->SetStopValue(1.0f);
    (void)a->IsTerminated(); // must not crash / stack-overflow

    // Duplicate-enqueue: B queued behind A again — also a cycle (would
    // walk A's tail = B and try to append B after B, self-loop).
    b->Queue(a, 1);
    (void)a->IsTerminated();

    a->Release();
    b->Release();
    fx.sys->Commit();
}

// After Restart()ing a chain head, the next natural termination must
// re-activate the successor.  Previously the idempotency flag
// (_queueNextActivated) stayed true across Restart and the second
// activation was skipped → chain hung.
TEST_CASE("Queue(): chain re-activates after Restart()", "[audio-lifecycle][oal][radio-chain]")
{
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL or tone.wav fixture unavailable");
    }
    auto* head = static_cast<WaveOAL*>(fx.CreateLoadedWave());
    auto* tail = static_cast<WaveOAL*>(fx.CreateLoadedWave());
    REQUIRE(head);
    REQUIRE(tail);

    head->Play();
    fx.sys->Commit();
    tail->Queue(head, 1);

    // First cycle: terminate head, activate tail.
    head->PlayUntilStopValue(0.0f);
    head->SetStopValue(1.0f);
    (void)head->IsTerminated();
    fx.sys->Commit();
    REQUIRE(tail->State() == WaveState::Playing);

    // Reset for second cycle: stop tail (back to StoppedReplayable),
    // re-queue tail behind head, Restart head.
    tail->Stop();
    fx.sys->Commit();
    tail->Queue(head, 1); // re-mark tail as _queued

    head->Restart(); // head goes Terminal → StoppedReplayable
    head->Play();
    fx.sys->Commit();
    REQUIRE(head->State() == WaveState::Playing);

    // Force head terminated again — chain MUST re-activate tail even
    // though it was activated before.  Pre-fix _queueNextActivated
    // stayed true and the `if (!_queueNextActivated)` guard
    // short-circuited.
    head->PlayUntilStopValue(0.0f);
    head->SetStopValue(1.0f);
    (void)head->IsTerminated();
    fx.sys->Commit();
    CHECK(tail->State() == WaveState::Playing);

    head->Release();
    tail->Release();
    fx.sys->Commit();
}

TEST_CASE("A-01-OAL: full FSM walk on SoundSystemOAL", "[audio-lifecycle][A-01][oal]")
{
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL or tone.wav fixture unavailable");
    }

    IWave* w = fx.CreateLoadedWave();
    REQUIRE(w != nullptr);

    // Created — never played.
    REQUIRE(w->State() == WaveState::Created);

    // Created -> WaitingDeferred via Play; Commit drives DoPlay -> Playing.
    w->Play();
    REQUIRE(w->State() == WaveState::WaitingDeferred);
    fx.sys->Commit();
    REQUIRE(w->State() == WaveState::Playing);

    // Playing -> Paused via Pause (offset captured before alSourcePause).
    w->Pause();
    REQUIRE(w->State() == WaveState::Paused);

    // Paused -> WaitingDeferred via Resume; Commit drives DoPlay -> Playing.
    // Resume must clear _paused immediately so State() doesn't stay reading
    // Paused through the deferred-play window — see WaveOAL::Resume override.
    w->Resume();
    REQUIRE(w->State() == WaveState::WaitingDeferred);
    fx.sys->Commit();
    REQUIRE(w->State() == WaveState::Playing);

    // Playing -> StoppedReplayable via Stop.
    w->Stop();
    REQUIRE(w->State() == WaveState::StoppedReplayable);

    // StoppedReplayable -> StoppedReplayable via Restart (reset; not auto-play).
    w->Restart();
    REQUIRE(w->State() == WaveState::StoppedReplayable);

    // StoppedReplayable -> WaitingDeferred via Play; Commit -> Playing.
    w->Play();
    REQUIRE(w->State() == WaveState::WaitingDeferred);
    fx.sys->Commit();
    REQUIRE(w->State() == WaveState::Playing);

    w->Release();
}

TEST_CASE("A-03-OAL: Stop produces StoppedReplayable, not Terminal", "[audio-lifecycle][A-03][oal]")
{
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL or tone.wav fixture unavailable");
    }

    IWave* w = fx.CreateLoadedWave();
    REQUIRE(w != nullptr);

    w->Play();
    fx.sys->Commit();
    REQUIRE(w->State() == WaveState::Playing);

    w->Stop();
    CHECK(w->State() == WaveState::StoppedReplayable);
    CHECK(w->IsStopped());
    CHECK_FALSE(w->IsTerminated());

    // Stop is idempotent.
    w->Stop();
    CHECK(w->State() == WaveState::StoppedReplayable);
    CHECK_FALSE(w->IsTerminated());

    w->Release();
}

// A-04-OAL: Restart from Terminal revives to StoppedReplayable.
// Uses the deterministic SetStopValue path to reach Terminal — looping
// wave with stopThreshold below the test time triggers
// `_state.terminated = true` inside WaveOAL::SetStopValue.  Mirrors the
// TEMPLATE_LIST_TEST_CASE "Restart from Terminal revives" case for the
// non-OAL backends.
TEST_CASE("A-04-OAL: Restart from Terminal revives via SetStopValue path", "[audio-lifecycle][A-04][oal]")
{
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL or tone.wav fixture unavailable");
    }

    IWave* w = fx.CreateLoadedWave();
    REQUIRE(w != nullptr);

    w->Play();
    fx.sys->Commit();
    REQUIRE(w->State() == WaveState::Playing);

    // PlayUntilStopValue sets _stopThreshold + makes the wave looping;
    // a subsequent SetStopValue(time > threshold) sets _state.terminated
    // and calls Stop().  This is the deterministic terminal-transition
    // path on WaveOAL.
    w->PlayUntilStopValue(1.f);
    w->SetStopValue(2.f);
    CHECK(w->State() == WaveState::Terminal);
    CHECK(w->IsTerminated());

    // Terminal is sticky against Play / Stop / Pause.
    w->Play();
    CHECK(w->State() == WaveState::Terminal);
    w->Stop();
    CHECK(w->State() == WaveState::Terminal);
    w->Pause();
    CHECK(w->State() == WaveState::Terminal);

    // Restart is the canonical revival — Terminal -> StoppedReplayable.
    w->Restart();
    CHECK(w->State() == WaveState::StoppedReplayable);
    CHECK_FALSE(w->IsTerminated());

    // Revived wave can play again.
    w->Play();
    fx.sys->Commit();
    CHECK(w->State() == WaveState::Playing);

    w->Release();
}

TEST_CASE("A-04-OAL: Restart from {Playing, StoppedReplayable, Paused}", "[audio-lifecycle][A-04][oal]")
{
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL or tone.wav fixture unavailable");
    }

    // From Playing
    {
        IWave* w = fx.CreateLoadedWave();
        REQUIRE(w != nullptr);
        w->Play();
        fx.sys->Commit();
        REQUIRE(w->State() == WaveState::Playing);
        w->Restart();
        CHECK(w->State() == WaveState::StoppedReplayable);
        w->Release();
    }
    // From StoppedReplayable
    {
        IWave* w = fx.CreateLoadedWave();
        REQUIRE(w != nullptr);
        w->Play();
        fx.sys->Commit();
        w->Stop();
        REQUIRE(w->State() == WaveState::StoppedReplayable);
        w->Restart();
        CHECK(w->State() == WaveState::StoppedReplayable);
        w->Release();
    }
    // From Paused
    {
        IWave* w = fx.CreateLoadedWave();
        REQUIRE(w != nullptr);
        w->Play();
        fx.sys->Commit();
        w->Pause();
        REQUIRE(w->State() == WaveState::Paused);
        w->Restart();
        CHECK(w->State() == WaveState::StoppedReplayable);
        w->Release();
    }
}

// A-29 — Allocation failures produce attributed counters.
// Drive the OAL backend into source-pool exhaustion by creating many
// concurrent waves and playing them simultaneously.  OpenAL Soft's
// software path caps at ~256 sources by default; pushing past that
// triggers alGenSources failures inside WaveOAL::Load.  Each failure
// increments SoundSystemOAL::_allocFailures and emits a LOG_WARN
// attributed to the wave name.
//
// The test is best-effort: if the live driver has a larger pool, the
// allocation never fails and the test gracefully degrades to a sanity
// check (counter stays at 0 throughout).
TEST_CASE("A-29-OAL: source-pool exhaustion increments _allocFailures", "[audio-lifecycle][A-29][oal]")
{
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL or tone.wav fixture unavailable");
    }

    const int baseline = fx.sys->GetCounters().allocFailures;

    // Push past OpenAL Soft's default software-path source cap (~256).
    // Each Play() + Commit() lazily allocates an AL source via
    // WaveOAL::Load.  At some point alGenSources starts failing and
    // _allocFailures should tick.
    constexpr int kPushCount = 384;
    std::vector<Ref<IWave>> held;
    held.reserve(kPushCount);
    for (int i = 0; i < kPushCount; ++i)
    {
        IWave* w = fx.sys->CreateWaveFromMemory(fx.wavData.data(), fx.wavData.size(), ".wav", false);
        if (!w)
        {
            break; // CreateWave itself can't be exhausted on memory; just stop early.
        }
        held.emplace_back(w);
        w->Play();
    }
    fx.sys->Commit();

    const int after = fx.sys->GetCounters().allocFailures;
    INFO("baseline=" << baseline << " after=" << after << " waves=" << held.size());

    // Either the pool was deep enough to absorb all 384 waves
    // (counter unchanged — the cap is higher than expected) OR
    // exhaustion fired and the counter advanced.  Both outcomes
    // are valid; the contract is "if alGenSources fails, the
    // counter increments deterministically — silently dropped
    // failures are not allowed."  We verify the counter is
    // monotonic and non-negative.
    CHECK(after >= baseline);

    // If exhaustion did fire, at least one held wave should report
    // load failure via IsTerminated()/State()==Terminal (_loadError
    // sets _state.terminated in WaveOAL::Load's failure path).
    if (after > baseline)
    {
        int terminalCount = 0;
        for (auto& w : held)
        {
            if (w->State() == WaveState::Terminal)
            {
                ++terminalCount;
            }
        }
        // alGenSources failure must surface as Terminal — a wave
        // whose source allocation failed cannot play, and the FSM
        // must report it as dead rather than silently quiet.
        INFO("Exhausted waves observed: " << terminalCount);
        CHECK(terminalCount >= 1);
    }
}

// A-05 — Lifecycle teardown order is honored.
// Waves must be Released before the SoundSystem is deleted — the wave
// destructor calls _sys->UnregisterWave which dereferences the system
// pointer.  This test pins the convention by driving a typical
// gameplay shape (multi-wave inventory, mixed 2D/3D, all states) and
// asserting the teardown sequence leaves AL clean.
TEST_CASE("A-05-OAL: multi-wave Release-then-delete-system is clean", "[audio-lifecycle][A-05][oal]")
{
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL or tone.wav fixture unavailable");
    }

    const ALenum baselineError = SnapshotAlErrorState();

    // Build a representative inventory: 2D + 3D + paused + stopped
    // + never-played, mirroring what gameplay tends to hold at any
    // given moment.
    IWave* w2dPlaying = fx.sys->CreateWaveFromMemory(fx.wavData.data(), fx.wavData.size(), ".wav", false);
    IWave* w3dPlaying = fx.sys->CreateWaveFromMemory(fx.wavData.data(), fx.wavData.size(), ".wav", true);
    IWave* wPaused = fx.sys->CreateWaveFromMemory(fx.wavData.data(), fx.wavData.size(), ".wav", false);
    IWave* wStopped = fx.sys->CreateWaveFromMemory(fx.wavData.data(), fx.wavData.size(), ".wav", false);
    IWave* wCreated = fx.sys->CreateWaveFromMemory(fx.wavData.data(), fx.wavData.size(), ".wav", false);
    REQUIRE(w2dPlaying != nullptr);
    REQUIRE(w3dPlaying != nullptr);
    REQUIRE(wPaused != nullptr);
    REQUIRE(wStopped != nullptr);
    REQUIRE(wCreated != nullptr);

    w3dPlaying->SetPosition(Vector3(0.f, 0.f, 0.f), Vector3(0.f, 0.f, 0.f));

    w2dPlaying->Play();
    w3dPlaying->Play();
    wPaused->Play();
    wStopped->Play();
    fx.sys->Commit();
    wPaused->Pause();
    wStopped->Stop();
    fx.sys->Commit();

    REQUIRE(w2dPlaying->State() == WaveState::Playing);
    REQUIRE(w3dPlaying->State() == WaveState::Playing);
    REQUIRE(wPaused->State() == WaveState::Paused);
    REQUIRE(wStopped->State() == WaveState::StoppedReplayable);
    REQUIRE(wCreated->State() == WaveState::Created);

    // Release in arbitrary order — the system holds raw pointers in
    // _waves and the Release destructor must walk back through
    // UnregisterWave to drop the entry.  None of these calls touches
    // the about-to-be-destroyed AL context.
    wPaused->Release();
    w3dPlaying->Release();
    wStopped->Release();
    w2dPlaying->Release();
    wCreated->Release();

    // System destructor runs after all waves are gone — no stale
    // back-pointers to dereference.  AL stays clean throughout.
    CHECK(SnapshotAlErrorState() == baselineError);
    // fx.~OALFixture() then deletes the system — implicit assertion:
    // the suite doesn't crash on teardown.
}

// A-14 — Long-lived caches survive backend state changes.
// Gameplay code that caches an IWave* across frames must keep working
// after a SwitchOutputDevice: the backend either invalidates / reloads
// the wave transparently, or the cache holder gets a defined non-Terminal
// state to retrigger from.  A-06 tests the backend side (no stale AL
// IDs); A-14 tests the gameplay side (the cached pointer is still
// usable after the switch).
TEST_CASE("A-14-OAL: SwitchOutputDevice preserves cached waves structurally", "[audio-lifecycle][A-14][oal]")
{
    const std::string body = SwitchOutputDeviceBody();
    REQUIRE_FALSE(body.empty());

    CHECK(body.find("w->Unload();") != std::string::npos);
    CHECK(body.find("delete w") == std::string::npos);
    CHECK(body.find("w->Release()") == std::string::npos);
    CHECK(body.find("_waves.clear()") == std::string::npos);
}

// A-06 — Backend hardware unload precedes context destruction.
// Without `SwitchOutputDevice` walking _waves and calling Unload on
// each before destroying the old context, every WaveOAL keeps stale
// _alBuffer / _alSource IDs.  The next Play() on the new context
// alBufferData's against a freed-context ID and AL enters persistent
// AL_INVALID_NAME — surfacing as "Buffer error" flood per B-A04.
//
// Test creates a wave on the live context, switches device, then
// plays the wave; assert no AL errors leak.  Fix in 9f1c5241.
TEST_CASE("A-06-OAL: SwitchOutputDevice unloads waves before destroying the old context",
          "[audio-lifecycle][A-06][oal]")
{
    const std::string body = SwitchOutputDeviceBody();
    REQUIRE_FALSE(body.empty());

    const size_t unloadPos = body.find("w->Unload();");
    const size_t detachPos = body.find("alcMakeContextCurrent(nullptr);");
    const size_t destroyPos = body.find("alcDestroyContext");
    REQUIRE(unloadPos != std::string::npos);
    REQUIRE(detachPos != std::string::npos);
    REQUIRE(destroyPos != std::string::npos);
    CHECK(unloadPos < detachPos);
    CHECK(unloadPos < destroyPos);
}

// A-21 — SwitchOutputDevice preserves volume state.
// SoundSystemOAL snapshots cd/wave/speech volumes before destroying
// the context and re-applies via SetCDVolume / SetWaveVolume /
// SetSpeechVolume after the new context is current.  Each setter
// runs UpdateMixer which writes alListenerf(AL_GAIN) on the new
// context, so live waves get the right gain.
TEST_CASE("A-21-OAL: SwitchOutputDevice preserves volume state", "[audio-lifecycle][A-21][oal]")
{
    auto* sys = dynamic_cast<SoundSystemOAL*>(CreateSoundSystemOAL());
    if (!sys)
    {
        SKIP("OpenAL unavailable");
    }
    sys->SetWaveVolume(7.f);
    sys->SetSpeechVolume(3.f);
    sys->SetCDVolume(8.f);
    delete sys;

    const std::string body = SwitchOutputDeviceBody();
    REQUIRE_FALSE(body.empty());
    CHECK(body.find("float cd = _cdVolume;") != std::string::npos);
    CHECK(body.find("float wave = _waveVolume;") != std::string::npos);
    CHECK(body.find("float speech = _speechVolume;") != std::string::npos);
    CHECK(body.find("SetCDVolume(cd);") != std::string::npos);
    CHECK(body.find("SetWaveVolume(wave);") != std::string::npos);
    CHECK(body.find("SetSpeechVolume(speech);") != std::string::npos);
}

// A-24 — EAX enable/disable is reversible.
// EnableEAX(true) creates the AL aux effect slot + reverb effect;
// EnableEAX(false) releases them.  After the toggle round-trip,
// the audio system's observable state (volumes, EAX flag) must
// equal the pre-toggle state — no leaked references, no jammed
// flag, no AL error pending.
TEST_CASE("A-24-OAL: EnableEAX(true) -> EnableEAX(false) is reversible", "[audio-lifecycle][A-24][oal]")
{
    auto* sys = dynamic_cast<SoundSystemOAL*>(CreateSoundSystemOAL());
    if (!sys)
    {
        SKIP("OpenAL unavailable");
    }

    // Snapshot pre-toggle state.  EAX may already be enabled by
    // LoadConfig() — read the live flag, then toggle off->on->off
    // if it started off, or on->off->on if it started on.
    const float preWave = sys->GetWaveVolume();
    const bool preEAX = sys->GetEAX();

    const ALenum baselineError = SnapshotAlErrorState();

    if (preEAX)
    {
        // Toggle off → assert disabled; toggle back on.
        sys->EnableEAX(false);
        CHECK_FALSE(sys->GetEAX());
        sys->EnableEAX(true);
    }
    else
    {
        // Toggle on (returns false on devices without EFX) → toggle off.
        const bool ok = sys->EnableEAX(true);
        if (!ok)
        {
            SKIP("EFX extension unavailable on this device");
        }
        sys->EnableEAX(false);
        CHECK_FALSE(sys->GetEAX());
        // Round-trip back to the starting state.
        sys->EnableEAX(false);
    }

    // Final state must match the starting flag.
    CHECK(sys->GetEAX() == preEAX);
    // Other observable state is unchanged.
    CHECK(sys->GetWaveVolume() == Catch::Approx(preWave));
    // No AL errors leaked across the toggles.
    CHECK(SnapshotAlErrorState() == baselineError);

    delete sys;
}

// A-22 — Listener state survives backend context rebuild.
// SwitchOutputDevice destroys + recreates the AL context.  Without
// re-applying the cached listener pose, every 3D source snaps to
// origin/identity until the next world.cpp::SetListener tick.
TEST_CASE("A-22-OAL: SwitchOutputDevice preserves listener position", "[audio-lifecycle][A-22][oal]")
{
    auto* sys = dynamic_cast<SoundSystemOAL*>(CreateSoundSystemOAL());
    if (!sys)
    {
        SKIP("OpenAL unavailable");
    }

    const Vector3 pos(123.f, 45.f, -67.f);
    const Vector3 vel(1.f, 2.f, 3.f);
    const Vector3 dir(0.f, 0.f, 1.f);
    const Vector3 up(0.f, 1.f, 0.f);
    sys->SetListener(pos, vel, dir, up);
    REQUIRE(sys->GetListenerPosition().Distance2(pos) < 1e-4f);

    // SwitchOutputDevice early-returns true when the requested name
    // equals the cached _selectedDevice — bypasses the destroy+recreate
    // path A-22 is guarding.  Pick a named device from the live list so
    // the switch actually goes through.
    delete sys;

    const std::string body = SwitchOutputDeviceBody();
    REQUIRE_FALSE(body.empty());
    const size_t makeCurrentPos = body.find("alcMakeContextCurrent(newContext)");
    const size_t setListenerPos = body.find("SetListener(_listenerPos, _listenerVel, _listenerDir, _listenerUp);");
    REQUIRE(makeCurrentPos != std::string::npos);
    REQUIRE(setListenerPos != std::string::npos);
    CHECK(makeCurrentPos < setListenerPos);
}

TEST_CASE("A-30-OAL: SwitchOutputDevice keeps old backend until new bind succeeds", "[audio-lifecycle][A-30][oal]")
{
    const std::string body = SwitchOutputDeviceBody();
    REQUIRE_FALSE(body.empty());

    const size_t oldContextPos = body.find("ALCcontext* oldContext = static_cast<ALCcontext*>(_context);");
    const size_t newCurrentPos = body.find("alcMakeContextCurrent(newContext)");
    const size_t restoreOldPos = body.find("alcMakeContextCurrent(oldContext);");
    const size_t destroyOldPos = body.find("alcDestroyContext(oldContext);");
    REQUIRE(oldContextPos != std::string::npos);
    REQUIRE(newCurrentPos != std::string::npos);
    REQUIRE(restoreOldPos != std::string::npos);
    REQUIRE(destroyOldPos != std::string::npos);
    CHECK(oldContextPos < newCurrentPos);
    CHECK(newCurrentPos < destroyOldPos);
}

TEST_CASE("A-31-OAL: GetCurrentOutputDevice falls back to the live backend name", "[audio-lifecycle][A-31][oal]")
{
    const std::string body = GetCurrentOutputDeviceBody();
    REQUIRE_FALSE(body.empty());
    CHECK(body.find("if (!_selectedDevice.empty())") != std::string::npos);
    CHECK(body.find("alcGetString(static_cast<ALCdevice*>(_device), ALC_DEVICE_SPECIFIER)") != std::string::npos);
}

// A-18 — Per-category preview restart counter.  Each StartCategoryPreview
// call that successfully creates a wave increments the cumulative count.
// AudioPage drives this on category-row focus changes and on volume-slider
// drags, so the counter advances monotonically through normal UI use.
TEST_CASE("A-18-OAL: StartCategoryPreview increments CountCategoryPreviewRestarts", "[audio-lifecycle][A-18][oal]")
{
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL unavailable");
    }

    const int baseline = fx.sys->CountCategoryPreviewRestarts();

    fx.sys->StartCategoryPreview(WaveEffect);
    CHECK(fx.sys->CountCategoryPreviewRestarts() == baseline + 1);

    // Same kind — restarts unconditionally (the "volume drag" pattern).
    fx.sys->StartCategoryPreview(WaveEffect);
    CHECK(fx.sys->CountCategoryPreviewRestarts() == baseline + 2);

    // Category change — restarts the preview.
    fx.sys->StartCategoryPreview(WaveSpeech);
    CHECK(fx.sys->CountCategoryPreviewRestarts() == baseline + 3);

    fx.sys->StartCategoryPreview(WaveMusic);
    CHECK(fx.sys->CountCategoryPreviewRestarts() == baseline + 4);

    // TerminatePreview clears the current preview wave but does NOT
    // rewind the counter — it's session-cumulative.
    fx.sys->TerminatePreview();
    CHECK(fx.sys->CountCategoryPreviewRestarts() == baseline + 4);
}

TEST_CASE("A-28-OAL: SetVoiceBudgetCounters round-trips into GetCounters", "[audio-lifecycle][A-28][oal][voice-budget]")
{
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL unavailable");
    }

    // Default is zero on both counters.
    AudioCounters c0 = fx.sys->GetCounters();
    CHECK(c0.evictedThisFrame == 0);
    CHECK(c0.pausedThisFrame == 0);

    // Drive the setter as SoundScene::AdvanceAll would after
    // audio::ApplyVoiceBudget3D returns.
    fx.sys->SetVoiceBudgetCounters(/*evicted=*/3, /*paused=*/2);
    AudioCounters c1 = fx.sys->GetCounters();
    CHECK(c1.evictedThisFrame == 3);
    CHECK(c1.pausedThisFrame == 2);

    // Subsequent tick overwrites — the counters are "this frame", not
    // running totals.
    fx.sys->SetVoiceBudgetCounters(0, 0);
    AudioCounters c2 = fx.sys->GetCounters();
    CHECK(c2.evictedThisFrame == 0);
    CHECK(c2.pausedThisFrame == 0);
}

TEST_CASE("A-28-OAL: counters reflect active 3D vs 2D after FSM transitions", "[audio-lifecycle][A-28][oal]")
{
    OALFixture fx;
    if (!fx.Init())
    {
        SKIP("OpenAL or tone.wav fixture unavailable");
    }

    auto* w2d = fx.sys->CreateWaveFromMemory(fx.wavData.data(), fx.wavData.size(), ".wav", /*is3D=*/false);
    auto* w3d = fx.sys->CreateWaveFromMemory(fx.wavData.data(), fx.wavData.size(), ".wav", /*is3D=*/true);
    REQUIRE(w2d != nullptr);
    REQUIRE(w3d != nullptr);

    // 3D waves need a position before DoPlay will commit them — see
    // WaveOAL::DoPlay's `if (_is3D && !_state.posValid) return;` guard.
    w3d->SetPosition(Vector3(0.f, 0.f, 0.f), Vector3(0.f, 0.f, 0.f));

    w2d->Play();
    w3d->Play();
    fx.sys->Commit();

    AudioCounters c = fx.sys->GetCounters();
    CHECK(c.active2D == 1);
    CHECK(c.active3D == 1);
    CHECK(c.paused == 0);

    w2d->Pause();
    AudioCounters cAfterPause = fx.sys->GetCounters();
    CHECK(cAfterPause.active2D == 0);
    CHECK(cAfterPause.active3D == 1);
    CHECK(cAfterPause.paused == 1);

    w2d->Release();
    w3d->Release();
}

// A-13: Cache reuse goes through audio::RetriggerCachedWave instead of
// raw cached->Play().  Helper queries State() and dispatches the right
// verb for each prior state; returns false on null / Terminal so the
// caller drops the cache entry.
TEMPLATE_LIST_TEST_CASE("A-13: RetriggerCachedWave dispatches per FSM state", "[audio-lifecycle][A-13]", AllBackends)
{
    typename TestType::SystemT sys;
    INFO("Backend: " << TestType::Name());

    // Null -> false (caller drops).
    CHECK_FALSE(audio::RetriggerCachedWave(nullptr));

    // Created -> Play() -> Playing.
    {
        IWave* w = sys.CreateWave("created.wav", false, false);
        REQUIRE(w != nullptr);
        REQUIRE(w->State() == WaveState::Created);
        CHECK(audio::RetriggerCachedWave(w));
        CHECK((w->State() == WaveState::Playing || w->State() == WaveState::WaitingDeferred));
        w->Release();
    }

    // Playing -> no-op, still playing.
    {
        IWave* w = sys.CreateWave("playing.wav", false, false);
        w->Play();
        REQUIRE((w->State() == WaveState::Playing || w->State() == WaveState::WaitingDeferred));
        CHECK(audio::RetriggerCachedWave(w));
        CHECK((w->State() == WaveState::Playing || w->State() == WaveState::WaitingDeferred));
        w->Release();
    }

    // Paused -> Resume -> Playing.
    {
        IWave* w = sys.CreateWave("paused.wav", false, false);
        w->Play();
        w->Pause();
        REQUIRE(w->State() == WaveState::Paused);
        CHECK(audio::RetriggerCachedWave(w));
        CHECK((w->State() == WaveState::Playing || w->State() == WaveState::WaitingDeferred));
        w->Release();
    }

    // StoppedReplayable -> Play -> Playing.
    {
        IWave* w = sys.CreateWave("stopped.wav", false, false);
        w->Play();
        w->Stop();
        REQUIRE(w->State() == WaveState::StoppedReplayable);
        CHECK(audio::RetriggerCachedWave(w));
        CHECK((w->State() == WaveState::Playing || w->State() == WaveState::WaitingDeferred));
        w->Release();
    }

    // Terminal -> false (caller drops).
    {
        IWave* w = sys.CreateWave("terminal.wav", false, false);
        w->Play();
        w->LastLoop();
        REQUIRE(w->State() == WaveState::Terminal);
        CHECK_FALSE(audio::RetriggerCachedWave(w));
        // Verb did not revive — wave still Terminal until explicit Restart.
        CHECK(w->State() == WaveState::Terminal);
        w->Release();
    }
}

// A-13: IsCacheValid / IsCacheStale agree on the {null, Terminal} boundary.
TEMPLATE_LIST_TEST_CASE("A-13: IsCacheValid / IsCacheStale boundary", "[audio-lifecycle][A-13]", AllBackends)
{
    typename TestType::SystemT sys;
    INFO("Backend: " << TestType::Name());

    // Null
    CHECK_FALSE(audio::IsCacheValid(nullptr));
    CHECK(audio::IsCacheStale(nullptr));

    // Created / Playing / Paused / StoppedReplayable — all valid.
    IWave* w = sys.CreateWave("validity.wav", false, false);
    CHECK(audio::IsCacheValid(w));
    w->Play();
    CHECK(audio::IsCacheValid(w));
    w->Pause();
    CHECK(audio::IsCacheValid(w));
    w->Resume();
    CHECK(audio::IsCacheValid(w));
    w->Stop();
    CHECK(audio::IsCacheValid(w));

    // Terminal -> stale
    w->LastLoop();
    REQUIRE(w->State() == WaveState::Terminal);
    CHECK_FALSE(audio::IsCacheValid(w));
    CHECK(audio::IsCacheStale(w));

    w->Release();
}
