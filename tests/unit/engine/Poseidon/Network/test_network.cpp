#include <catch2/catch_test_macros.hpp>

#include <Poseidon/AI/AI.hpp>
#include <Poseidon/Network/Network.hpp>

TEST_CASE("network.hpp compiles", "[network]")
{
    SUCCEED("header included successfully");
}

TEST_CASE("Network game state enum values", "[network]")
{
    REQUIRE(NGSNone == 0);
    REQUIRE(NGSCreating == 1);
    REQUIRE(NGSCreate == 2);
}

TEST_CASE("Network constants", "[network]")
{
    REQUIRE(NO_PLAYER == 0);
    REQUIRE(AI_PLAYER == 1);
}

TEST_CASE("MissionHeader joinInProgress defaults to false", "[network][jip]")
{
    MissionHeader header;
    REQUIRE(header.joinInProgress == false);
}

TEST_CASE("MissionHeader joinInProgress can be set", "[network][jip]")
{
    MissionHeader header;
    header.joinInProgress = true;
    REQUIRE(header.joinInProgress == true);
}
