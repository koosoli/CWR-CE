#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Graphics/Rendering/Draw/SpecLods.hpp>
#include <Poseidon/Graphics/Rendering/Draw/Font.hpp>
#include <Poseidon/Graphics/Rendering/Draw/RandomShape.hpp>

using namespace Poseidon;
TEST_CASE("SpecLods: LOD constants", "[Rendering][Draw]")
{
    REQUIRE(SPEC_LOD == 1e15);
    REQUIRE(MEMORY_SPEC == 1e15);
    REQUIRE(LANDCONTACT_SPEC == 2e15);
    REQUIRE(HITPOINTS_SPEC == 5e15);
    REQUIRE(GEOMETRY_SPEC == 1e13);
}

TEST_CASE("SpecLods: MaterialSection enum values", "[Rendering][Draw]")
{
    REQUIRE(MSShining == 200);
    REQUIRE(MSInShadow == 201);
    REQUIRE(MSFullLighted == 203);
}

TEST_CASE("font.hpp compiles", "[Rendering][Draw]")
{
    REQUIRE(true);
}

TEST_CASE("randomShape.hpp compiles", "[Rendering][Draw]")
{
    REQUIRE(true);
}
