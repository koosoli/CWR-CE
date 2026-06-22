#include <Poseidon/World/World.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("world compiles", "[world]")
{
    REQUIRE(sizeof(World) > 0);
}
