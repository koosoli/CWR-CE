#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>
#include <Poseidon/Graphics/Rendering/Shape/ClipShape.hpp>
#include <Poseidon/Graphics/Rendering/Draw/SpecLods.hpp>

TEST_CASE("Shape: LODShape default construction", "[Shape]")
{
    LODShape shape;
    REQUIRE(shape.NLevels() == 0);
}

TEST_CASE("clipShape.hpp compiles", "[Shape]")
{
    REQUIRE(true);
}

TEST_CASE("LODShape: FindSpecLevel skips dropped null LOD slots", "[Shape][LOD]")
{
    LODShape lod;
    lod.AddShape(new Shape(), 0.0f);
    lod.AddShape(new Shape(), VIEW_PILOT);

    REQUIRE(lod.FindSpecLevel(VIEW_PILOT) == 1);

    lod.ChangeShape(1, nullptr);

    REQUIRE(lod.FindSpecLevel(VIEW_PILOT) == -1);
    REQUIRE(lod.FindLevel(10.0f) == 0);
}
