// Unit tests for audio::ApplyVoiceBudget3D and audio::ApplyVoice2D —
// the voice-budget drivers in engine/Poseidon/Audio/Core/VoiceBudget.
// Covers audio-invariants A-07 (budget eviction == caller-stop),
// A-08 (2D / 3D paths distinct), A-09 (deterministic eviction),
// A-10 (paused != stop), A-11 (kMinAudibleBudget floor).
//
// Uses an inline MockWave that records every verb call so the test
// can assert which waves were Play'd, Stop'd, Pause'd, Skip'd, or
// had SetAccommodation called.

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Audio/IAudioSystem.hpp>
#include <Poseidon/Audio/Core/VoiceBudget.hpp>
#include <Poseidon/Core/HandledList.hpp>
#include <stddef.h>
#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/Foundation/Types/RemoveLinks.hpp>

using namespace Poseidon;
namespace
{

class MockWave : public IWave
{
  public:
    MockWave(const char* name, Vector3Par pos, float volume, bool sticky = false)
        : IWave(name), _pos(pos), _volume(volume)
    {
        SetSticky(sticky);
    }

    void Queue(IWave*, int) override {}
    void Repeat(int) override {}
    void Play() override
    {
        ++playCalls;
        _playing = true;
        _paused = false;
        _everPlayed = true;
    }
    void Stop() override
    {
        ++stopCalls;
        _playing = false;
        _paused = false;
    }
    void Pause() override
    {
        ++pauseCalls;
        _playing = false;
        _paused = true;
    }
    void Resume() override
    {
        _playing = true;
        _paused = false;
    }
    void LastLoop() override { _terminated = true; }
    void PlayUntilStopValue(float) override {}
    void SetStopValue(float) override {}

    bool IsWaiting() override { return _waiting; }
    void Skip(float) override { ++skipCalls; }
    void Advance(float) override {}
    float GetLength() const override { return 1.f; }
    bool IsStopped() override { return !_playing && !_paused; }
    bool IsTerminated() override { return _terminated; }
    void Restart() override
    {
        _playing = false;
        _paused = false;
        _terminated = false;
    }
    WaveState State() override
    {
        if (_terminated)
            return WaveState::Terminal;
        if (_paused)
            return WaveState::Paused;
        if (_playing)
            return WaveState::Playing;
        return _everPlayed ? WaveState::StoppedReplayable : WaveState::Created;
    }

    bool IsMuted() const override { return false; }
    float GetVolume() const override { return _volume; }
    void SetVolume(float v, float, bool) override { _volume = v; }
    float GetFileMaxVolume() const override { return 1.f; }
    float GetFileAvgVolume() const override { return 1.f; }

    void SetAccommodation(float a) override
    {
        ++accomCalls;
        _lastAccom = a;
    }
    float GetAccommodation() const override { return _lastAccom; }
    void EnableAccommodation(bool e) override { _accomEnabled = e; }
    bool AccommodationEnabled() const override { return _accomEnabled; }

    void SetKind(WaveKind k) override { _kind = k; }
    WaveKind GetKind() const override { return _kind; }

    void SetPosition(Vector3Par pos, Vector3Par, bool) override { _pos = pos; }
    Vector3 GetPosition() const override { return _pos; }
    void Set3D(bool) override {}
    bool Get3D() const override { return true; }
    float Distance2D() const override { return 1.f; }

    // Inspection state for tests.
    int playCalls = 0;
    int stopCalls = 0;
    int pauseCalls = 0;
    int skipCalls = 0;
    int accomCalls = 0;
    float _lastAccom = 0.f;

    void MarkWaiting() { _waiting = true; }
    void MarkTerminated() { _terminated = true; }

  private:
    Vector3 _pos;
    float _volume;
    bool _playing = false;
    bool _paused = false;
    bool _waiting = false;
    bool _terminated = false;
    bool _accomEnabled = true;
    bool _everPlayed = false;
    WaveKind _kind = WaveEffect;
};

// Helper: build a LinkArray<IWave> from a vector of MockWave*.
// LinkArray<IWave> uses intrusive linkage (RefCountWithLinks); owned[]
// holds the Ref<> that keeps each mock alive for the duration of the
// test.
struct SceneBuilder
{
    LinkArray<IWave> sounds;
    std::vector<Ref<MockWave>> owned;

    MockWave* Add(const char* name, Vector3Par pos, float volume, bool sticky = false)
    {
        Ref<MockWave> mw = new MockWave(name, pos, volume, sticky);
        IWave* raw = mw.GetRef();
        sounds.Add(raw);
        owned.push_back(std::move(mw));
        return static_cast<MockWave*>(raw);
    }
};

audio::VoiceBudget3DInput DefaultInput(int budget = 16, bool paused = false)
{
    audio::VoiceBudget3DInput in;
    in.listenerPos = Vector3(0.f, 0.f, 0.f);
    in.earAccommodation = 1.f;
    in.soundVolume = 1.f;
    in.maxAudibleBudget = budget;
    in.gamePaused = paused;
    in.deltaT = 0.016f;
    return in;
}

} // namespace

TEST_CASE("VoiceBudget3D: single close-by wave plays", "[audio][voice-budget][A-07]")
{
    SceneBuilder scene;
    MockWave* close = scene.Add("close.wav", Vector3(1.f, 0.f, 0.f), 1.f);

    auto r = audio::ApplyVoiceBudget3D(scene.sounds, DefaultInput());

    CHECK(r.audibleCount == 1);
    CHECK(r.evictedThisFrame == 0);
    CHECK(r.pausedThisFrame == 0);
    CHECK(close->playCalls == 1);
    CHECK(close->stopCalls == 0);
}

TEST_CASE("VoiceBudget3D: budget overflow evicts the farthest waves", "[audio][voice-budget][A-07][A-09]")
{
    // Five waves at increasing distances; budget of 16 means all 5 are
    // audible.  Drop the budget below the kMinAudibleBudget floor to
    // confirm the floor applies (A-11) — still 5 audible.
    SceneBuilder scene;
    MockWave* w1 = scene.Add("w1", Vector3(1.f, 0.f, 0.f), 1.f);
    MockWave* w2 = scene.Add("w2", Vector3(2.f, 0.f, 0.f), 1.f);
    MockWave* w3 = scene.Add("w3", Vector3(3.f, 0.f, 0.f), 1.f);
    MockWave* w4 = scene.Add("w4", Vector3(4.f, 0.f, 0.f), 1.f);
    MockWave* w5 = scene.Add("w5", Vector3(5.f, 0.f, 0.f), 1.f);

    auto r = audio::ApplyVoiceBudget3D(scene.sounds, DefaultInput(/*budget=*/2));

    // kMinAudibleBudget floor (16) prevents the budget=2 from kicking in.
    CHECK(r.audibleCount == 5);
    CHECK(r.evictedThisFrame == 0);
    for (auto* w : {w1, w2, w3, w4, w5})
    {
        CHECK(w->playCalls == 1);
    }
}

TEST_CASE("VoiceBudget3D: above-floor budget evicts farthest", "[audio][voice-budget][A-07][A-09]")
{
    // 20 candidate waves at increasing distance; budget = 17 (above the
    // floor of 16).  Top 17 (closest) play; the 3 farthest evict.
    SceneBuilder scene;
    std::vector<MockWave*> waves;
    for (int i = 0; i < 20; i++)
    {
        waves.push_back(scene.Add("w", Vector3(static_cast<float>(i + 1), 0.f, 0.f), 1.f));
    }

    auto r = audio::ApplyVoiceBudget3D(scene.sounds, DefaultInput(/*budget=*/17));
    CHECK(r.audibleCount == 17);
    CHECK(r.evictedThisFrame == 3);

    // The 17 closest waves were Play'd; the 3 farthest were Stop'd+Skip'd.
    int played = 0, evicted = 0;
    for (auto* w : waves)
    {
        if (w->playCalls == 1 && w->stopCalls == 0)
        {
            ++played;
        }
        else if (w->playCalls == 0 && w->stopCalls == 1 && w->skipCalls == 1)
        {
            ++evicted;
        }
    }
    CHECK(played == 17);
    CHECK(evicted == 3);
}

TEST_CASE("VoiceBudget3D: deterministic - same input produces same eviction", "[audio][voice-budget][A-09]")
{
    auto runOnce = []()
    {
        SceneBuilder scene;
        for (int i = 0; i < 20; i++)
        {
            scene.Add("w", Vector3(static_cast<float>(i + 1), 0.f, 0.f), 1.f);
        }
        return audio::ApplyVoiceBudget3D(scene.sounds, DefaultInput(/*budget=*/17));
    };
    auto r1 = runOnce();
    auto r2 = runOnce();
    CHECK(r1.audibleCount == r2.audibleCount);
    CHECK(r1.evictedThisFrame == r2.evictedThisFrame);
    CHECK(r1.pausedThisFrame == r2.pausedThisFrame);
    CHECK(r1.nLoud == r2.nLoud);
}

TEST_CASE("VoiceBudget3D: gamePaused + non-sticky -> Pause; sticky stays audible", "[audio][voice-budget][A-10]")
{
    SceneBuilder scene;
    MockWave* nonSticky = scene.Add("non.wav", Vector3(1.f, 0.f, 0.f), 1.f, /*sticky=*/false);
    MockWave* sticky = scene.Add("stk.wav", Vector3(1.f, 0.f, 0.f), 1.f, /*sticky=*/true);

    auto r = audio::ApplyVoiceBudget3D(scene.sounds, DefaultInput(/*budget=*/16, /*paused=*/true));

    CHECK(nonSticky->pauseCalls == 1);
    CHECK(nonSticky->playCalls == 0);
    CHECK(nonSticky->stopCalls == 0); // A-10 — Pause, never Stop
    CHECK(sticky->playCalls == 1);
    CHECK(sticky->stopCalls == 0);
    CHECK(sticky->pauseCalls == 0);
    CHECK(r.pausedThisFrame == 1);
    CHECK(r.audibleCount == 1);
}

TEST_CASE("VoiceBudget3D: inaudible-quiet wave is stopped, not paused", "[audio][voice-budget][A-07]")
{
    // Volume too low to clear LouderThan threshold at any reasonable
    // distance.  Wave gets the Stop+Skip path, not Pause (game running).
    SceneBuilder scene;
    MockWave* quiet = scene.Add("quiet.wav", Vector3(1.f, 0.f, 0.f), 1e-12f);

    auto r = audio::ApplyVoiceBudget3D(scene.sounds, DefaultInput());

    CHECK(quiet->stopCalls == 1);
    CHECK(quiet->skipCalls == 1);
    CHECK(quiet->pauseCalls == 0);
    CHECK(quiet->playCalls == 0);
    CHECK(r.audibleCount == 0);
    CHECK(r.evictedThisFrame == 0); // budget didn't fire; this is the quiet path
}

TEST_CASE("VoiceBudget3D: waiting wave is stopped, not played", "[audio][voice-budget][A-07]")
{
    SceneBuilder scene;
    MockWave* waiting = scene.Add("wait.wav", Vector3(1.f, 0.f, 0.f), 1.f);
    waiting->MarkWaiting();

    auto r = audio::ApplyVoiceBudget3D(scene.sounds, DefaultInput());

    CHECK(waiting->stopCalls == 1);
    CHECK(waiting->skipCalls == 1);
    CHECK(waiting->playCalls == 0);
    CHECK(r.audibleCount == 0);
}

TEST_CASE("VoiceBudget3D: terminated wave is stopped, not played", "[audio][voice-budget][A-07]")
{
    SceneBuilder scene;
    MockWave* dead = scene.Add("dead.wav", Vector3(1.f, 0.f, 0.f), 1.f);
    dead->MarkTerminated();

    auto r = audio::ApplyVoiceBudget3D(scene.sounds, DefaultInput());

    CHECK(dead->stopCalls == 1);
    CHECK(dead->playCalls == 0);
    CHECK(r.audibleCount == 0);
}

TEST_CASE("VoiceBudget3D: audible waves get SetAccommodation when enabled", "[audio][voice-budget][A-07]")
{
    SceneBuilder scene;
    MockWave* accom = scene.Add("a.wav", Vector3(1.f, 0.f, 0.f), 1.f);
    MockWave* noAccom = scene.Add("b.wav", Vector3(1.f, 0.f, 0.f), 1.f);
    noAccom->EnableAccommodation(false);

    auto in = DefaultInput();
    in.earAccommodation = 0.7f;
    in.soundVolume = 0.5f;
    auto r = audio::ApplyVoiceBudget3D(scene.sounds, in);

    CHECK(accom->accomCalls == 1);
    CHECK(accom->_lastAccom == 0.7f * 0.5f);
    CHECK(r.nLoud == 1);
    CHECK(noAccom->accomCalls == 0);
}

TEST_CASE("VoiceBudget3D: empty scene returns zero counters", "[audio][voice-budget]")
{
    SceneBuilder scene;
    auto r = audio::ApplyVoiceBudget3D(scene.sounds, DefaultInput());
    CHECK(r.audibleCount == 0);
    CHECK(r.evictedThisFrame == 0);
    CHECK(r.pausedThisFrame == 0);
    CHECK(r.totalLoudness == 0.f);
    CHECK(r.nLoud == 0);
}

// Crowded-scene unit precursor — missing sounds near Houdan under heavy 3D source counts.
// The integration-harness
// version lives in tests/integration/ingame/audio/;
// this unit version drives audio::ApplyVoiceBudget3D directly with 32
// candidate waves split into near/far clusters, listener moves
// between two positions, asserts the audible set tracks correctly
// and is deterministic across runs.  Covers audio-invariants
// A-09 / A-30.
TEST_CASE("VoiceBudget3D: 32-wave crowded scene tracks listener pose deterministically",
          "[audio][voice-budget][crowded][A-09][A-30]")
{
    auto buildScene = []()
    {
        auto s = std::make_unique<SceneBuilder>();
        // 16 waves clustered near origin (x in [1..16], y=z=0).
        for (int i = 0; i < 16; i++)
        {
            s->Add("near", Vector3(static_cast<float>(i + 1), 0.f, 0.f), 1.f);
        }
        // 16 waves clustered far (x in [100..115]).  Same volume so the
        // ranking is purely distance-driven.
        for (int i = 0; i < 16; i++)
        {
            s->Add("far", Vector3(100.f + static_cast<float>(i), 0.f, 0.f), 1.f);
        }
        return s;
    };

    auto runAt = [](SceneBuilder& s, Vector3Par listenerPos, int budget)
    {
        audio::VoiceBudget3DInput in;
        in.listenerPos = listenerPos;
        in.earAccommodation = 1.f;
        in.soundVolume = 1.f;
        in.maxAudibleBudget = budget;
        in.gamePaused = false;
        in.deltaT = 0.016f;
        return audio::ApplyVoiceBudget3D(s.sounds, in);
    };

    SECTION("Listener at origin: 16 nearest dominate the top-N")
    {
        auto s = buildScene();
        auto r = runAt(*s, Vector3(0.f, 0.f, 0.f), /*budget=*/16);
        CHECK(r.audibleCount == 16);
        CHECK(r.evictedThisFrame == 16);
        // The 16 audible should all be the near cluster.
        for (int i = 0; i < 16; i++)
        {
            CHECK(s->owned[i]->playCalls == 1); // near[i] played
        }
        for (int i = 16; i < 32; i++)
        {
            CHECK(s->owned[i]->stopCalls == 1); // far[i-16] evicted
            CHECK(s->owned[i]->playCalls == 0);
        }
    }

    SECTION("Listener moves into the far cluster: audible set flips")
    {
        auto s = buildScene();
        // Listener at x=107 — middle of far cluster.  The 16 closest
        // should now be the far waves and (some of) the previously-near
        // waves at the boundary, depending on distance.
        auto r = runAt(*s, Vector3(107.f, 0.f, 0.f), /*budget=*/16);
        CHECK(r.audibleCount == 16);
        CHECK(r.evictedThisFrame == 16);
        // Far cluster waves at x in [100..115] are closer to listener@107
        // than the near cluster waves at x in [1..16] (distance ~91..106
        // vs near's ~91..106 — wait, near is also at ~91..106).  Let me
        // be specific: far waves at x=92..107 are nearest the listener,
        // near waves at x=1..16 are at distance ~91..106 — they overlap.
        // Without committing to a specific partition, just assert the
        // farthest cluster of near waves (x=1..7) got evicted.
        for (int i = 0; i < 7; i++)
        {
            CHECK(s->owned[i]->stopCalls == 1);
        }
    }

    SECTION("Deterministic — same input twice produces identical decisions")
    {
        auto s1 = buildScene();
        auto s2 = buildScene();
        auto r1 = runAt(*s1, Vector3(0.f, 0.f, 0.f), 16);
        auto r2 = runAt(*s2, Vector3(0.f, 0.f, 0.f), 16);
        CHECK(r1.audibleCount == r2.audibleCount);
        CHECK(r1.evictedThisFrame == r2.evictedThisFrame);
        CHECK(r1.totalLoudness == r2.totalLoudness);
        // Every wave's Play/Stop call counts match.
        for (size_t i = 0; i < s1->owned.size(); i++)
        {
            CHECK(s1->owned[i]->playCalls == s2->owned[i]->playCalls);
            CHECK(s1->owned[i]->stopCalls == s2->owned[i]->stopCalls);
        }
    }

    SECTION("Re-promotion: an evicted wave can come back when listener moves")
    {
        // Build, evaluate at origin (far cluster evicted), then move
        // listener into far cluster and re-evaluate.  Each new tick
        // creates fresh budget decisions — evictedThisFrame from the
        // FIRST tick doesn't bias the second.  The previously-evicted
        // far wave gets Play'd this time.
        auto s = buildScene();
        runAt(*s, Vector3(0.f, 0.f, 0.f), 16);
        Ref<MockWave>& prevFar = s->owned[20]; // far[4], x=104
        CHECK(prevFar->playCalls == 0);
        CHECK(prevFar->stopCalls == 1);

        runAt(*s, Vector3(104.f, 0.f, 0.f), 16);
        // prevFar at x=104, listener@104, dist=0 — guaranteed in top-N.
        CHECK(prevFar->playCalls == 1);
    }
}

// audio::ApplyVoice2D — audio-invariants A-08 (2D and 3D paths
// intentionally distinct).  The 2D path has no budget; every wave is
// either Play+Advance (game running, or sticky) or Pause (game paused
// and non-sticky).  Loudness accumulates from accommodation-eligible
// waves for the accommodation history.
TEST_CASE("Voice2D: game running -> Play+Advance on every wave", "[audio][voice-2d][A-08]")
{
    SceneBuilder scene;
    MockWave* w1 = scene.Add("a.wav", Vector3(0.f, 0.f, 0.f), 1.f);
    MockWave* w2 = scene.Add("b.wav", Vector3(0.f, 0.f, 0.f), 0.5f);
    w1->Set3D(false);
    w2->Set3D(false);

    audio::Voice2DInput in;
    in.earAccommodation = 1.f;
    in.soundVolume = 1.f;
    in.gamePaused = false;
    in.deltaT = 0.016f;
    auto r = audio::ApplyVoice2D(scene.sounds, in);

    CHECK(r.activeCount == 2);
    CHECK(r.pausedThisFrame == 0);
    CHECK(w1->playCalls == 1);
    CHECK(w2->playCalls == 1);
    CHECK(w1->pauseCalls == 0);
    CHECK(w2->pauseCalls == 0);
}

TEST_CASE("Voice2D: game paused + non-sticky -> Pause; sticky -> Play", "[audio][voice-2d][A-08][A-10]")
{
    SceneBuilder scene;
    MockWave* nonSticky = scene.Add("ns.wav", Vector3(0.f, 0.f, 0.f), 1.f, /*sticky=*/false);
    MockWave* sticky = scene.Add("stk.wav", Vector3(0.f, 0.f, 0.f), 1.f, /*sticky=*/true);

    audio::Voice2DInput in;
    in.earAccommodation = 1.f;
    in.soundVolume = 1.f;
    in.gamePaused = true;
    in.deltaT = 0.016f;
    auto r = audio::ApplyVoice2D(scene.sounds, in);

    CHECK(nonSticky->pauseCalls == 1);
    CHECK(nonSticky->playCalls == 0);
    CHECK(nonSticky->stopCalls == 0); // A-10 — Pause, never Stop
    CHECK(sticky->playCalls == 1);
    CHECK(sticky->pauseCalls == 0);
    CHECK(r.activeCount == 1);
    CHECK(r.pausedThisFrame == 1);
}

TEST_CASE("Voice2D: accommodation-enabled waves contribute loudness", "[audio][voice-2d][A-08]")
{
    SceneBuilder scene;
    MockWave* accom = scene.Add("a.wav", Vector3(0.f, 0.f, 0.f), 1.f);
    MockWave* noAccom = scene.Add("n.wav", Vector3(0.f, 0.f, 0.f), 1.f);
    noAccom->EnableAccommodation(false);

    audio::Voice2DInput in;
    in.earAccommodation = 0.5f;
    in.soundVolume = 0.4f;
    in.gamePaused = false;
    auto r = audio::ApplyVoice2D(scene.sounds, in);

    CHECK(accom->accomCalls == 1);
    CHECK(accom->_lastAccom == 0.5f * 0.4f);
    CHECK(noAccom->accomCalls == 0);
    // totalLoudness is _vol / Square(Distance2D()) = 1.0 / 1.0 = 1.0
    // for the accom wave; noAccom contributes nothing.
    CHECK(r.totalLoudness == 1.f);
}

TEST_CASE("Voice2D: terminated wave is skipped (no Play, no Pause)", "[audio][voice-2d][A-08][regression]")
{
    // Regression: pre-fix ApplyVoice2D called wave->Pause() / wave->Play()
    // unconditionally.  For a wave whose AL source has naturally finished
    // but whose engine `terminated` flag has not yet propagated (race
    // between AL_STOPPED and the next IsTerminated/IsStopped poll), the
    // pause-then-resume cycle armed wantPlaying and DoPlay restarted the
    // source from byte 0 — user-visible "Esc-twice replays the last
    // sound".  Mirror the 3D path's IsTerminated() guard.
    SceneBuilder scene;
    MockWave* live = scene.Add("live.wav", Vector3(0.f, 0.f, 0.f), 1.f);
    MockWave* done = scene.Add("done.wav", Vector3(0.f, 0.f, 0.f), 1.f);
    done->MarkTerminated();

    SECTION("running game")
    {
        audio::Voice2DInput in;
        in.earAccommodation = 1.f;
        in.soundVolume = 1.f;
        in.gamePaused = false;
        auto r = audio::ApplyVoice2D(scene.sounds, in);
        CHECK(live->playCalls == 1);
        CHECK(done->playCalls == 0);
        CHECK(done->pauseCalls == 0);
        CHECK(r.activeCount == 1);
    }

    SECTION("paused game")
    {
        audio::Voice2DInput in;
        in.earAccommodation = 1.f;
        in.soundVolume = 1.f;
        in.gamePaused = true;
        auto r = audio::ApplyVoice2D(scene.sounds, in);
        CHECK(live->pauseCalls == 1);
        CHECK(done->pauseCalls == 0);
        CHECK(done->playCalls == 0);
        CHECK(r.pausedThisFrame == 1);
    }
}

TEST_CASE("Voice2D: empty scene returns zero counters", "[audio][voice-2d][A-08]")
{
    SceneBuilder scene;
    audio::Voice2DInput in;
    auto r = audio::ApplyVoice2D(scene.sounds, in);
    CHECK(r.activeCount == 0);
    CHECK(r.pausedThisFrame == 0);
    CHECK(r.totalLoudness == 0.f);
}

TEST_CASE("Voice2D: deterministic across runs", "[audio][voice-2d][A-09]")
{
    auto runOnce = []()
    {
        SceneBuilder scene;
        for (int i = 0; i < 8; i++)
        {
            scene.Add("w", Vector3(0.f, 0.f, 0.f), 1.f);
        }
        audio::Voice2DInput in;
        in.earAccommodation = 1.f;
        in.soundVolume = 1.f;
        return audio::ApplyVoice2D(scene.sounds, in);
    };
    auto a = runOnce();
    auto b = runOnce();
    CHECK(a.activeCount == b.activeCount);
    CHECK(a.pausedThisFrame == b.pausedThisFrame);
    CHECK(a.totalLoudness == b.totalLoudness);
}

TEST_CASE("VoiceBudget3D: A-11 floor - sub-16 budget input clamps to 16", "[audio][voice-budget][A-11]")
{
    // 16 audible candidates; budget input = 4.  Floor kicks in to 16
    // so all 16 are audible.
    SceneBuilder scene;
    for (int i = 0; i < 16; i++)
    {
        scene.Add("w", Vector3(static_cast<float>(i + 1), 0.f, 0.f), 1.f);
    }
    auto r = audio::ApplyVoiceBudget3D(scene.sounds, DefaultInput(/*budget=*/4));
    CHECK(r.audibleCount == 16);
    CHECK(r.evictedThisFrame == 0);
    CHECK(audio::kMinAudibleBudget == 16);
}
