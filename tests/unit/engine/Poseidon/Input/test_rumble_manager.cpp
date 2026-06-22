#include <Poseidon/Input/RumbleManager.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

// --- RumbleEffect tests ---

using namespace Poseidon;
TEST_CASE("RumbleEffect: default is inactive", "[input][rumble]")
{
    RumbleEffect e;
    REQUIRE_FALSE(e.IsActive());
    REQUIRE(e.Evaluate(16.0f) == Catch::Approx(0.0f));
}

TEST_CASE("RumbleEffect: Start/Evaluate ramps correctly", "[input][rumble]")
{
    RumbleEffect e;
    e.Start(1.0f, 0.0f, 100.0f);
    REQUIRE(e.IsActive());

    // At 50% through, linear interp: 1.0 + (0.0 - 1.0) * 0.5 = 0.5
    float mag = e.Evaluate(50.0f);
    REQUIRE(mag == Catch::Approx(0.5f));
}

TEST_CASE("RumbleEffect: expires after duration", "[input][rumble]")
{
    RumbleEffect e;
    e.Start(1.0f, 0.0f, 100.0f);

    e.Evaluate(50.0f);
    float mag = e.Evaluate(50.0f); // elapsed = 100, at duration boundary
    REQUIRE(mag == Catch::Approx(0.0f));
    REQUIRE_FALSE(e.IsActive());
}

TEST_CASE("RumbleEffect: Stop clears state", "[input][rumble]")
{
    RumbleEffect e;
    e.Start(1.0f, 0.0f, 200.0f);
    e.Evaluate(10.0f);

    e.Stop();
    REQUIRE_FALSE(e.IsActive());
    REQUIRE(e.Evaluate(10.0f) == Catch::Approx(0.0f));
}

TEST_CASE("RumbleEffect: negative magnitudes are abs'd", "[input][rumble]")
{
    RumbleEffect e;
    e.Start(-0.8f, -0.2f, 100.0f);
    // beginMagnitude should be 0.8, endMagnitude 0.2
    float mag = e.Evaluate(50.0f); // t=0.5: 0.8 + (0.2 - 0.8)*0.5 = 0.5
    REQUIRE(mag == Catch::Approx(0.5f));
}

// --- RumbleManager tests ---

TEST_CASE("RumbleManager: engine rumble sets continuous magnitude", "[input][rumble]")
{
    RumbleManager rm;
    rm.SetEngineRumble(0.6f);
    REQUIRE(rm.GetEngineMagnitude() == Catch::Approx(0.6f));
}

TEST_CASE("RumbleManager: engine rumble clamps to [0, 1]", "[input][rumble]")
{
    RumbleManager rm;
    rm.SetEngineRumble(1.5f);
    REQUIRE(rm.GetEngineMagnitude() == Catch::Approx(1.0f));

    rm.SetEngineRumble(-0.8f);
    REQUIRE(rm.GetEngineMagnitude() == Catch::Approx(0.8f));
}

TEST_CASE("RumbleManager: PlayEffect creates timed effect", "[input][rumble]")
{
    RumbleManager rm;
    rm.PlayEffect(1.0f, 0.0f, 100.0f);
    REQUIRE(rm.IsEffectActive());
}

TEST_CASE("RumbleManager: Evaluate blends engine + effect into left/right motors", "[input][rumble]")
{
    RumbleManager rm;
    rm.SetEngineRumble(0.5f);
    rm.PlayEffect(1.0f, 0.0f, 100.0f);

    float left = 0.0f, right = 0.0f;
    // At t=0: effect evaluates first step. With deltaMs=50, t=0.5, effectMag=0.5
    rm.Evaluate(50.0f, left, right);

    // left = max(0.5*0.7, 0.5*0.5) = max(0.35, 0.25) = 0.35
    // right = max(0.5*0.3, 0.5*1.0) = max(0.15, 0.5) = 0.5
    REQUIRE(left == Catch::Approx(0.35f));
    REQUIRE(right == Catch::Approx(0.5f));
}

TEST_CASE("RumbleManager: no effect + no engine gives zero motors", "[input][rumble]")
{
    RumbleManager rm;
    float left = 1.0f, right = 1.0f;
    rm.Evaluate(16.0f, left, right);
    REQUIRE(left == Catch::Approx(0.0f));
    REQUIRE(right == Catch::Approx(0.0f));
}

TEST_CASE("RumbleManager: weak signals below threshold are zeroed", "[input][rumble]")
{
    RumbleManager rm;
    // Engine at 0.1 → left = 0.1*0.7=0.07 (<0.10 → 0), right = 0.1*0.3=0.03 (<0.05 → 0)
    rm.SetEngineRumble(0.1f);
    float left = 0.0f, right = 0.0f;
    rm.Evaluate(16.0f, left, right);
    REQUIRE(left == Catch::Approx(0.0f));
    REQUIRE(right == Catch::Approx(0.0f));
}

TEST_CASE("RumbleManager: signals between threshold and floor are floored", "[input][rumble]")
{
    RumbleManager rm;
    // Engine at 0.2 → left = 0.2*0.7=0.14 (>=0.10, <0.20 → floor to 0.20)
    //                  right = 0.2*0.3=0.06 (>=0.05, <0.10 → floor to 0.10)
    rm.SetEngineRumble(0.2f);
    float left = 0.0f, right = 0.0f;
    rm.Evaluate(16.0f, left, right);
    REQUIRE(left == Catch::Approx(0.20f));
    REQUIRE(right == Catch::Approx(0.10f));
}

TEST_CASE("RumbleManager: StopAll clears everything", "[input][rumble]")
{
    RumbleManager rm;
    rm.SetEngineRumble(0.8f);
    rm.PlayEffect(1.0f, 0.0f, 200.0f);

    rm.StopAll();
    REQUIRE(rm.GetEngineMagnitude() == Catch::Approx(0.0f));
    REQUIRE_FALSE(rm.IsEffectActive());

    float left = 1.0f, right = 1.0f;
    rm.Evaluate(16.0f, left, right);
    REQUIRE(left == Catch::Approx(0.0f));
    REQUIRE(right == Catch::Approx(0.0f));
}
