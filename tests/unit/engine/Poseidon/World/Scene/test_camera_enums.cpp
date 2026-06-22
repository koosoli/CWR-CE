#include <catch2/catch_test_macros.hpp>
#include <Poseidon/World/Scene/Camera/CamEffects.hpp>

using namespace Poseidon;
TEST_CASE("CamEffectName enum values", "[scene][camera]")
{
    REQUIRE(CamEffectNone == 0);
    REQUIRE(CamEffectStatic == 1);
    REQUIRE(CamEffectTerminate == 12);
    REQUIRE(NCamEffects == 13);
}

TEST_CASE("CamEffectPosition enum values", "[scene][camera]")
{
    REQUIRE(CamEffectTop == 0);
    REQUIRE(CamEffectLeft == 1);
    REQUIRE(CamEffectBottom == 13);
    REQUIRE(NCamEffectPositions == 14);
}
