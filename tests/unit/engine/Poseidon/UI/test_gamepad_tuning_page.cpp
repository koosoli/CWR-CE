#include <Poseidon/UI/Options/GamepadTuningPage.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Poseidon;
TEST_CASE("GamepadTuningPage deadzone conversion clamps into the supported range", "[UI][GamepadTuningPage]")
{
    CHECK(GamepadTuningPage::DeadzoneToPercent(-0.2f) == 0);
    CHECK(GamepadTuningPage::DeadzoneToPercent(0.0f) == 0);
    CHECK(GamepadTuningPage::DeadzoneToPercent(0.25f) == 50);
    CHECK(GamepadTuningPage::DeadzoneToPercent(0.5f) == 100);
    CHECK(GamepadTuningPage::DeadzoneToPercent(0.8f) == 100);

    CHECK(GamepadTuningPage::PercentToDeadzone(-10) == Catch::Approx(0.0f));
    CHECK(GamepadTuningPage::PercentToDeadzone(50) == Catch::Approx(0.25f));
    CHECK(GamepadTuningPage::PercentToDeadzone(100) == Catch::Approx(0.5f));
    CHECK(GamepadTuningPage::PercentToDeadzone(120) == Catch::Approx(0.5f));
}

TEST_CASE("GamepadTuningPage look sensitivity conversion clamps into the supported range", "[UI][GamepadTuningPage]")
{
    CHECK(GamepadTuningPage::LookSensitivityToPercent(-1.0f) == 0);
    CHECK(GamepadTuningPage::LookSensitivityToPercent(0.1f) == 0);
    CHECK(GamepadTuningPage::LookSensitivityToPercent(1.0f) == 18);
    CHECK(GamepadTuningPage::LookSensitivityToPercent(5.0f) == 100);
    CHECK(GamepadTuningPage::LookSensitivityToPercent(7.0f) == 100);

    CHECK(GamepadTuningPage::PercentToLookSensitivity(-10) == Catch::Approx(0.1f));
    CHECK(GamepadTuningPage::PercentToLookSensitivity(0) == Catch::Approx(0.1f));
    CHECK(GamepadTuningPage::PercentToLookSensitivity(50) == Catch::Approx(2.55f));
    CHECK(GamepadTuningPage::PercentToLookSensitivity(100) == Catch::Approx(5.0f));
    CHECK(GamepadTuningPage::PercentToLookSensitivity(120) == Catch::Approx(5.0f));
}
