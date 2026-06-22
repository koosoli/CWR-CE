#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Graphics/Rendering/Colors.hpp>
#include <Poseidon/Graphics/Rendering/ColorsFloat.hpp>
#include <Poseidon/Graphics/Rendering/Lighting/Material.hpp>
#include <Poseidon/Graphics/Rendering/Lighting/Lights.hpp>
#include <Poseidon/Input/KeyLights.hpp>

TEST_CASE("ColorP: construct from RGB floats", "[Rendering][Colors]")
{
    ColorP c(0.5f, 0.25f, 0.75f);
    REQUIRE(c.R() == Catch::Approx(0.5f));
    REQUIRE(c.G() == Catch::Approx(0.25f));
    REQUIRE(c.B() == Catch::Approx(0.75f));
    REQUIRE(c.A() == Catch::Approx(1.0f));
}

TEST_CASE("ColorP: construct from RGBA floats", "[Rendering][Colors]")
{
    ColorP c(1.0f, 0.0f, 0.0f, 0.5f);
    REQUIRE(c.R() == Catch::Approx(1.0f));
    REQUIRE(c.A() == Catch::Approx(0.5f));
}

TEST_CASE("ColorP: R8/G8/B8 return 0-255 range", "[Rendering][Colors]")
{
    ColorP c(1.0f, 0.0f, 0.5f);
    REQUIRE(c.R8() == 255);
    REQUIRE(c.G8() == 0);
    // 0.5 * 255 = 127.5 -> toInt rounds to nearest-even -> 128.  Regression
    // guard: Poseidon::toInt(int) once hid the global toInt(float) here,
    // making this truncate to 127 (see Core/Types.hpp `using ::toInt`).
    REQUIRE(c.B8() == 128);
}

TEST_CASE("ColorP: multiplication by scalar", "[Rendering][Colors]")
{
    ColorP c(1.0f, 0.5f, 0.25f);
    ColorP r = c * 0.5f;
    REQUIRE(r.R() == Catch::Approx(0.5f));
    REQUIRE(r.G() == Catch::Approx(0.25f));
    REQUIRE(r.B() == Catch::Approx(0.125f));
}

TEST_CASE("material.hpp compiles", "[Rendering][Material]")
{
    REQUIRE(true);
}

TEST_CASE("lights.hpp compiles", "[Rendering][Lights]")
{
    REQUIRE(true);
}

TEST_CASE("KeyLights.hpp compiles", "[Rendering][Lights]")
{
    REQUIRE(true);
}
