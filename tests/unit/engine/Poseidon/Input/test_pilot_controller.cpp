#include <Poseidon/Input/VehicleControls.hpp>
#include <Poseidon/Input/PilotController.hpp>
#include <Poseidon/Input/InfantryController.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <memory>

// TestController — concrete PilotController that returns hardcoded values
// for verifying polymorphism without requiring the input system.

using namespace Poseidon;
class TestController : public PilotController
{
  public:
    VehicleControls Poll(float /*deltaT*/) override
    {
        VehicleControls c;
        c.throttle = 0.5f;
        c.steering = -0.3f;
        c.fire = true;
        return c;
    }
    InputContext GetContext() const override { return InputContext::CarDriver; }
};

TEST_CASE("VehicleControls: default construction zeroes all fields", "[input][pilot]")
{
    VehicleControls ctrl;

    REQUIRE(ctrl.throttle == Catch::Approx(0.0f));
    REQUIRE(ctrl.steering == Catch::Approx(0.0f));
    REQUIRE(ctrl.steeringR == Catch::Approx(0.0f));
    REQUIRE(ctrl.pitch == Catch::Approx(0.0f));
    REQUIRE(ctrl.roll == Catch::Approx(0.0f));
    REQUIRE(ctrl.yaw == Catch::Approx(0.0f));
    REQUIRE(ctrl.collective == Catch::Approx(0.0f));
    REQUIRE(ctrl.lookX == Catch::Approx(0.0f));
    REQUIRE(ctrl.lookY == Catch::Approx(0.0f));

    REQUIRE_FALSE(ctrl.fire);
    REQUIRE_FALSE(ctrl.toggleWeapons);
    REQUIRE_FALSE(ctrl.reloadMagazine);
    REQUIRE_FALSE(ctrl.lockTarget);
    REQUIRE_FALSE(ctrl.headlights);
    REQUIRE_FALSE(ctrl.optics);
    REQUIRE_FALSE(ctrl.lookAround);
    REQUIRE_FALSE(ctrl.turbo);
    REQUIRE_FALSE(ctrl.slow);
}

TEST_CASE("VehicleControls: Reset clears all fields", "[input][pilot]")
{
    VehicleControls ctrl;
    ctrl.throttle = 0.8f;
    ctrl.steering = -0.5f;
    ctrl.pitch = 1.0f;
    ctrl.fire = true;
    ctrl.turbo = true;
    ctrl.lookX = 0.3f;

    ctrl.Reset();

    REQUIRE(ctrl.throttle == Catch::Approx(0.0f));
    REQUIRE(ctrl.steering == Catch::Approx(0.0f));
    REQUIRE(ctrl.pitch == Catch::Approx(0.0f));
    REQUIRE(ctrl.lookX == Catch::Approx(0.0f));
    REQUIRE_FALSE(ctrl.fire);
    REQUIRE_FALSE(ctrl.turbo);
}

TEST_CASE("VehicleControls: can hold mixed analog and digital values", "[input][pilot]")
{
    VehicleControls ctrl;
    ctrl.throttle = 1.0f;
    ctrl.steering = -1.0f;
    ctrl.collective = 0.75f;
    ctrl.fire = true;
    ctrl.headlights = true;
    ctrl.lookX = -0.5f;
    ctrl.slow = true;

    REQUIRE(ctrl.throttle == Catch::Approx(1.0f));
    REQUIRE(ctrl.steering == Catch::Approx(-1.0f));
    REQUIRE(ctrl.collective == Catch::Approx(0.75f));
    REQUIRE(ctrl.fire);
    REQUIRE(ctrl.headlights);
    REQUIRE(ctrl.lookX == Catch::Approx(-0.5f));
    REQUIRE(ctrl.slow);
}

TEST_CASE("PilotController: default curves are Stick/Trigger", "[input][pilot]")
{
    TestController tc;

    REQUIRE(tc.GetSteeringCurve().deadzone == Catch::Approx(0.21f));
    REQUIRE(tc.GetThrottleCurve().deadzone == Catch::Approx(0.10f));
    REQUIRE(tc.GetPitchCurve().deadzone == Catch::Approx(0.21f));
    REQUIRE(tc.GetRollCurve().deadzone == Catch::Approx(0.21f));
    REQUIRE(tc.GetYawCurve().deadzone == Catch::Approx(0.21f));
}

TEST_CASE("PilotController: set/get curve round-trips", "[input][pilot]")
{
    TestController tc;

    ResponseCurve custom;
    custom.type = CurveType::Quadratic;
    custom.deadzone = 0.05f;
    custom.sensitivity = 1.5f;

    tc.SetSteeringCurve(custom);
    REQUIRE(tc.GetSteeringCurve().type == CurveType::Quadratic);
    REQUIRE(tc.GetSteeringCurve().deadzone == Catch::Approx(0.05f));
    REQUIRE(tc.GetSteeringCurve().sensitivity == Catch::Approx(1.5f));

    tc.SetPitchCurve(ResponseCurve::FlightStick());
    REQUIRE(tc.GetPitchCurve().type == CurveType::Quadratic);
    REQUIRE(tc.GetPitchCurve().deadzone == Catch::Approx(0.15f));
}

TEST_CASE("PilotController: TestController returns correct context and values", "[input][pilot]")
{
    TestController tc;
    REQUIRE(tc.GetContext() == InputContext::CarDriver);

    VehicleControls ctrl = tc.Poll(0.016f);
    REQUIRE(ctrl.throttle == Catch::Approx(0.5f));
    REQUIRE(ctrl.steering == Catch::Approx(-0.3f));
    REQUIRE(ctrl.fire);
    REQUIRE_FALSE(ctrl.turbo);
}

TEST_CASE("PilotController: polymorphism through base pointer", "[input][pilot]")
{
    std::unique_ptr<PilotController> controller = std::make_unique<TestController>();
    REQUIRE(controller->GetContext() == InputContext::CarDriver);

    VehicleControls ctrl = controller->Poll(0.016f);
    REQUIRE(ctrl.throttle == Catch::Approx(0.5f));
    REQUIRE(ctrl.fire);
}

TEST_CASE("InfantryController: returns Infantry context", "[input][pilot]")
{
    InfantryController ic;
    REQUIRE(ic.GetContext() == InputContext::Infantry);
}

TEST_CASE("InfantryController: is a PilotController", "[input][pilot]")
{
    InfantryController ic;
    PilotController* base = &ic;
    REQUIRE(base->GetContext() == InputContext::Infantry);
}

TEST_CASE("ResponseCurve presets applied to controller axes", "[input][pilot]")
{
    TestController tc;

    tc.SetSteeringCurve(ResponseCurve::FlightStick());
    tc.SetThrottleCurve(ResponseCurve::Trigger(0.05f));

    // FlightStick has Quadratic + 0.15 deadzone
    REQUIRE(tc.GetSteeringCurve().Apply(0.1f) == Catch::Approx(0.0f));
    REQUIRE(tc.GetSteeringCurve().Apply(1.0f) == Catch::Approx(1.0f));

    // Trigger with custom deadzone 0.05
    REQUIRE(tc.GetThrottleCurve().Apply(0.03f) == Catch::Approx(0.0f));
    REQUIRE(tc.GetThrottleCurve().Apply(1.0f) == Catch::Approx(1.0f));
}
