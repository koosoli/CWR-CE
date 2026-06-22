#include <Poseidon/Input/ResponseCurve.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

using namespace Poseidon;
TEST_CASE("ResponseCurve: linear with no deadzone passes through", "[input][curve]")
{
    ResponseCurve c = ResponseCurve::Default();
    REQUIRE(c.Apply(0.0f) == Catch::Approx(0.0f));
    REQUIRE(c.Apply(0.5f) == Catch::Approx(0.5f));
    REQUIRE(c.Apply(1.0f) == Catch::Approx(1.0f));
}

TEST_CASE("ResponseCurve: linear with deadzone zeroes small values", "[input][curve]")
{
    ResponseCurve c;
    c.deadzone = 0.2f;
    REQUIRE(c.Apply(0.0f) == Catch::Approx(0.0f));
    REQUIRE(c.Apply(0.1f) == Catch::Approx(0.0f));
    REQUIRE(c.Apply(0.19f) == Catch::Approx(0.0f));
}

TEST_CASE("ResponseCurve: deadzone remaps [dz..1] to [0..1]", "[input][curve]")
{
    ResponseCurve c;
    c.deadzone = 0.2f;
    // At deadzone boundary, output should be near 0
    REQUIRE(c.Apply(0.2f) == Catch::Approx(0.0f).margin(0.01f));
    // At max, output should be 1
    REQUIRE(c.Apply(1.0f) == Catch::Approx(1.0f));
    // Midpoint: (0.6 - 0.2) / (1.0 - 0.2) = 0.5
    REQUIRE(c.Apply(0.6f) == Catch::Approx(0.5f));
}

TEST_CASE("ResponseCurve: saturation clamps output", "[input][curve]")
{
    ResponseCurve c;
    c.saturation = 0.8f;
    // At saturation point, output should be 1
    REQUIRE(c.Apply(0.8f) == Catch::Approx(1.0f));
    // Beyond saturation, still clamped to 1
    REQUIRE(c.Apply(1.0f) == Catch::Approx(1.0f));
}

TEST_CASE("ResponseCurve: quadratic curve shapes values", "[input][curve]")
{
    ResponseCurve c;
    c.type = CurveType::Quadratic;
    REQUIRE(c.Apply(0.5f) == Catch::Approx(0.25f));
    REQUIRE(c.Apply(1.0f) == Catch::Approx(1.0f));
    REQUIRE(c.Apply(0.0f) == Catch::Approx(0.0f));
}

TEST_CASE("ResponseCurve: cubic curve shapes values", "[input][curve]")
{
    ResponseCurve c;
    c.type = CurveType::Cubic;
    REQUIRE(c.Apply(0.5f) == Catch::Approx(0.125f));
    REQUIRE(c.Apply(1.0f) == Catch::Approx(1.0f));
}

TEST_CASE("ResponseCurve: SCurve has smooth shape", "[input][curve]")
{
    ResponseCurve c;
    c.type = CurveType::SCurve;
    REQUIRE(c.Apply(0.0f) == Catch::Approx(0.0f));
    // smoothstep at 0.5: 3*(0.25) - 2*(0.125) = 0.75 - 0.25 = 0.5
    REQUIRE(c.Apply(0.5f) == Catch::Approx(0.5f));
    REQUIRE(c.Apply(1.0f) == Catch::Approx(1.0f));
}

TEST_CASE("ResponseCurve: exponential with exponent=3 matches cubic", "[input][curve]")
{
    ResponseCurve c;
    c.type = CurveType::Exponential;
    c.exponent = 3.0f;
    REQUIRE(c.Apply(0.5f) == Catch::Approx(0.125f).margin(0.001f));
    REQUIRE(c.Apply(1.0f) == Catch::Approx(1.0f));
}

TEST_CASE("ResponseCurve: negative values preserve sign", "[input][curve]")
{
    ResponseCurve c;
    c.type = CurveType::Quadratic;
    REQUIRE(c.Apply(-0.5f) == Catch::Approx(-0.25f));
    REQUIRE(c.Apply(-1.0f) == Catch::Approx(-1.0f));
}

TEST_CASE("ResponseCurve: clamping at boundaries", "[input][curve]")
{
    ResponseCurve c;
    // Values beyond ±1 are clamped to input range first
    REQUIRE(c.Apply(2.0f) == Catch::Approx(1.0f));
    REQUIRE(c.Apply(-2.0f) == Catch::Approx(-1.0f));
}

TEST_CASE("ResponseCurve: sensitivity multiplier", "[input][curve]")
{
    ResponseCurve c;
    c.sensitivity = 0.5f;
    REQUIRE(c.Apply(1.0f) == Catch::Approx(0.5f));
    REQUIRE(c.Apply(0.5f) == Catch::Approx(0.25f));
}

TEST_CASE("ResponseCurve: Stick() has correct deadzone", "[input][curve]")
{
    constexpr auto stick = ResponseCurve::Stick();
    REQUIRE(stick.deadzone == Catch::Approx(0.21f));
    REQUIRE(stick.type == CurveType::Linear);
}

TEST_CASE("ResponseCurve: FlightStick() uses Quadratic", "[input][curve]")
{
    constexpr auto fs = ResponseCurve::FlightStick();
    REQUIRE(fs.type == CurveType::Quadratic);
    REQUIRE(fs.deadzone == Catch::Approx(0.15f));
}

TEST_CASE("ResponseCurve: zero range returns 0", "[input][curve]")
{
    ResponseCurve c;
    c.deadzone = 0.5f;
    c.saturation = 0.5f; // range = 0
    REQUIRE(c.Apply(0.7f) == Catch::Approx(0.0f));
    REQUIRE(c.Apply(1.0f) == Catch::Approx(0.0f));
}
