#include <Poseidon/World/MapTypes.hpp>
#include <catch2/catch_test_macros.hpp>

using Poseidon::MapBuilding;
using Poseidon::MapHouse;
using Poseidon::MapTree;
using Poseidon::NMapTypes;

TEST_CASE("MapType enum values", "[io]")
{
    REQUIRE(MapTree == 0);
    REQUIRE(MapBuilding == 3);
    REQUIRE(MapHouse == 4);
    REQUIRE(NMapTypes > 0);
    REQUIRE(NMapTypes == 24);
}
