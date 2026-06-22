#include <Poseidon/Input/ComboDetector.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

using namespace Poseidon;
TEST_CASE("ComboDetector: initial state is Idle", "[input][combo]")
{
    ComboDetector cd;
    REQUIRE(cd.GetState() == ComboState::Idle);
    REQUIRE(cd.IsDoubleTap() == false);
    REQUIRE(cd.IsHold() == false);
    REQUIRE(cd.HoldDurationMs() == Catch::Approx(0.0f));
}

TEST_CASE("ComboDetector: single press-release doesn't trigger double-tap", "[input][combo]")
{
    ComboDetector cd;
    cd.OnPress();
    cd.Update(50.0f);
    cd.OnRelease();
    cd.Update(16.0f);
    REQUIRE(cd.IsDoubleTap() == false);
}

TEST_CASE("ComboDetector: two quick taps triggers double-tap", "[input][combo]")
{
    ComboDetector cd;
    // First tap
    cd.OnPress();
    cd.Update(30.0f);
    cd.OnRelease();
    cd.Update(16.0f);

    // Second tap within window
    cd.OnPress();
    REQUIRE(cd.IsDoubleTap() == true);
}

TEST_CASE("ComboDetector: two taps outside window doesn't trigger", "[input][combo]")
{
    ComboDetector cd;
    // First tap
    cd.OnPress();
    cd.Update(30.0f);
    cd.OnRelease();

    // Wait beyond the double-tap window
    cd.Update(350.0f);

    // Second tap — too late
    cd.OnPress();
    REQUIRE(cd.IsDoubleTap() == false);
}

TEST_CASE("ComboDetector: hold detection triggers after threshold", "[input][combo]")
{
    ComboDetector cd;
    cd.OnPress();
    REQUIRE(cd.IsHold() == false);

    // Accumulate hold time past threshold (400ms default)
    cd.Update(200.0f);
    REQUIRE(cd.IsHold() == false);
    cd.Update(250.0f);
    REQUIRE(cd.IsHold() == true);
}

TEST_CASE("ComboDetector: hold duration tracks time", "[input][combo]")
{
    ComboDetector cd;
    cd.OnPress();
    cd.Update(100.0f);
    cd.Update(150.0f);
    REQUIRE(cd.HoldDurationMs() == Catch::Approx(250.0f));
}

TEST_CASE("ComboDetector: double-tap flag clears on next Update", "[input][combo]")
{
    ComboDetector cd;
    // First tap
    cd.OnPress();
    cd.Update(30.0f);
    cd.OnRelease();
    cd.Update(16.0f);

    // Second tap → double-tap detected
    cd.OnPress();
    REQUIRE(cd.IsDoubleTap() == true);

    // Next update clears the flag
    cd.Update(16.0f);
    REQUIRE(cd.IsDoubleTap() == false);
}

TEST_CASE("ComboDetector: Reset clears all state", "[input][combo]")
{
    ComboDetector cd;
    cd.OnPress();
    cd.Update(500.0f);
    REQUIRE(cd.IsHold() == true);

    cd.Reset();
    REQUIRE(cd.GetState() == ComboState::Idle);
    REQUIRE(cd.IsDoubleTap() == false);
    REQUIRE(cd.IsHold() == false);
    REQUIRE(cd.HoldDurationMs() == Catch::Approx(0.0f));
}

TEST_CASE("ComboDetector: rapid press-release-press sequence", "[input][combo]")
{
    ComboDetector cd;
    // Rapid: press, tiny hold, release, press immediately
    cd.OnPress();
    cd.Update(10.0f);
    cd.OnRelease();
    cd.Update(5.0f);
    cd.OnPress();
    REQUIRE(cd.IsDoubleTap() == true);
}

TEST_CASE("ComboDetector: long hold then release returns to Idle", "[input][combo]")
{
    ComboDetector cd;
    cd.OnPress();
    cd.Update(500.0f);
    REQUIRE(cd.GetState() == ComboState::Holding);

    cd.OnRelease();
    // Held longer than doubleTapWindow, so should go to Idle (not WaitSecondTap)
    REQUIRE(cd.GetState() == ComboState::Idle);
}
