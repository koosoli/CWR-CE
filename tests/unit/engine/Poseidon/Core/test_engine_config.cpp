// Tests for EngineConfig.  Persistent settings (texture caps, LOD,
// memory heaps, etc.) are owned by value; per-frame runtime flags and
// computed scene state are aliased into RuntimeFlags / EngineState.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Core/Config/EngineConfig.hpp>
#include <Poseidon/Core/RuntimeFlags.hpp>
#include <Poseidon/Core/EngineState.hpp>
#include <string>

TEST_CASE("EngineConfig owns persistent settings with sane defaults", "[core][config]")
{
    Poseidon::RuntimeFlags rf;
    Poseidon::EngineState es;
    Poseidon::EngineConfig config(rf, es);

    SECTION("Texture caps")
    {
        CHECK(config.maxCockText == 4096);
        CHECK(config.maxLandText == 4096);
        CHECK(config.maxObjText == 4096);
        CHECK(config.maxAnimText == 2048);
        CHECK(config.autoDropText == 1);
    }

    SECTION("Memory heap sizes")
    {
        CHECK(config.heapSize == 512);
        CHECK(config.fileHeapSize == 256);
        // Process budget limits default to -1 = auto: a near-OOM backstop derived
        // from physical RAM at apply time (0 = explicit unlimited, >0 = explicit MB).
        CHECK(config.memorySoftLimitMB == -1);
        CHECK(config.memoryHardLimitMB == -1);
    }

    SECTION("Lighting")
    {
        CHECK(config.maxLights == 16);
        CHECK(config.lights == 7); // LIGHT_EXPLO | LIGHT_MISSILE | LIGHT_STATIC
    }

    SECTION("Display")
    {
        CHECK(config.wantW == 800);
        CHECK(config.wantH == 600);
        CHECK(config.wantBpp == 32);
        CHECK(config.displayMode == "borderless");
    }

    SECTION("Field writes round-trip through the same instance")
    {
        config.wantW = 1920;
        config.wantH = 1080;
        config.maxObjText = 2048;
        config.lodCoef = 15.5f;
        CHECK(config.wantW == 1920);
        CHECK(config.wantH == 1080);
        CHECK(config.maxObjText == 2048);
        CHECK(config.lodCoef == Catch::Approx(15.5f));
    }
}

TEST_CASE("EngineState seeds tire/tank track decay constants", "[core][config][tracks]")
{
    // Regression for tire-track / tank-track decals never fading.
    //
    // Original main.cpp set Glob.config.trackTimeToLive=60 and
    // invTrackTimeToLive=1/60 at engine boot.  When ported to
    // EngineState the init line was lost and both fields defaulted to
    // 0.0f, with no setter anywhere — so in tracks.cpp:
    //
    //   _alpha -= deltaT * invTrackTimeToLive;   // *= 0  →  never fades
    //   howOld >= trackTimeToLive * 0.1          // >= 0  →  always true
    //
    // The first line freezes jeep tracks at spawn alpha 0.2 (and tank
    // tracks at 0.7) forever, accumulating over a sortie; the second
    // forces a new TrackStep shape every frame.  Both symptoms read as
    // "decals are way stronger than the 1999 original."
    //
    // Broken-state probe: set both fields back to 0.0f in
    // EngineState.hpp — these CHECKs fail (0 > 0 is false, and the
    // invTrackTimeToLive ≈ 1/60 check fails outright).

    Poseidon::EngineState es;
    CHECK(es.trackTimeToLive == Catch::Approx(60.0f));
    CHECK(es.invTrackTimeToLive == Catch::Approx(1.0f / 60.0f));
    CHECK(es.trackTimeToLive > 0.0f);
    CHECK(es.invTrackTimeToLive > 0.0f);
}

TEST_CASE("EngineConfig aliases RuntimeFlags + EngineState", "[core][config][wiring]")
{
    Poseidon::RuntimeFlags rf;
    Poseidon::EngineState es;
    Poseidon::EngineConfig view(rf, es);

    SECTION("Writes to RuntimeFlags surface through EngineConfig")
    {
        rf.hostMultiplayer = true;
        rf.dedicatedServer = true;
        rf.checkInitAndExit = true;
        rf.noLandscape = true;
        rf.hideCursor = true;
        rf.blood = false;

        CHECK(view.doCreateServer == true);
        CHECK(view.doCreateDedicatedServer == true);
        CHECK(view.checkInitAndExit == true);
        CHECK(view.noLandscape == true);
        CHECK(view.hideCursor == true);
        CHECK(view.blood == false);
    }

    SECTION("Writes to EngineState surface through EngineConfig")
    {
        es.horizontZ = 1200.0f;
        es.objectsZ = 800.0f;
        es.tacticalZ = 1100.0f;
        es.radarZ = 3000.0f;
        es.shadowsZ = 300.0f;

        CHECK(view.horizontZ == Catch::Approx(1200.0f));
        CHECK(view.objectsZ == Catch::Approx(800.0f));
        CHECK(view.tacticalZ == Catch::Approx(1100.0f));
        CHECK(view.radarZ == Catch::Approx(3000.0f));
        CHECK(view.shadowsZ == Catch::Approx(300.0f));
    }

    SECTION("EngineConfig writes to alias fields update backing store")
    {
        view.blood = false;
        view.hideCursor = true;
        view.noLandscape = true;

        CHECK(rf.blood == false);
        CHECK(rf.hideCursor == true);
        CHECK(rf.noLandscape == true);
    }
}
