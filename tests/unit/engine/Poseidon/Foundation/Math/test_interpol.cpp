#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Foundation/Math/Interpol.hpp>

TEST_CASE("Lint - linear interpolation", "[utility][interpol]")
{
    // y = 2x on [0, 1, 2, 3]
    float x[] = {0.0f, 1.0f, 2.0f, 3.0f};
    float y[] = {0.0f, 2.0f, 4.0f, 6.0f};

    SECTION("interpolate at exact nodes")
    {
        REQUIRE(Lint(x, y, 4, 0.0f) == Catch::Approx(0.0f));
        REQUIRE(Lint(x, y, 4, 1.0f) == Catch::Approx(2.0f));
        REQUIRE(Lint(x, y, 4, 3.0f) == Catch::Approx(6.0f));
    }

    SECTION("interpolate between nodes")
    {
        REQUIRE(Lint(x, y, 4, 0.5f) == Catch::Approx(1.0f));
        REQUIRE(Lint(x, y, 4, 1.5f) == Catch::Approx(3.0f));
        REQUIRE(Lint(x, y, 4, 2.5f) == Catch::Approx(5.0f));
    }

    SECTION("clamp below range")
    {
        REQUIRE(Lint(x, y, 4, -1.0f) == Catch::Approx(0.0f));
    }

    SECTION("clamp above range")
    {
        REQUIRE(Lint(x, y, 4, 5.0f) == Catch::Approx(6.0f));
    }
}

TEST_CASE("Lint - non-uniform spacing", "[utility][interpol]")
{
    float x[] = {0.0f, 1.0f, 4.0f};
    float y[] = {0.0f, 3.0f, 12.0f};

    SECTION("midpoint of first segment")
    {
        REQUIRE(Lint(x, y, 3, 0.5f) == Catch::Approx(1.5f));
    }

    SECTION("midpoint of second segment")
    {
        REQUIRE(Lint(x, y, 3, 2.5f) == Catch::Approx(7.5f));
    }
}
