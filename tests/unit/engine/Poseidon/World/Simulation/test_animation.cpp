#include <catch2/catch_test_macros.hpp>
#include <Poseidon/World/Simulation/Animation/Animation.hpp>
#include <Poseidon/World/Simulation/Animation/RtAnimation.hpp>

TEST_CASE("animation.hpp - compile check", "[simulation][animation]")
{
    SUCCEED("animation.hpp included successfully");
}

TEST_CASE("rtAnimation.hpp - compile check", "[simulation][rtAnimation]")
{
    SUCCEED("rtAnimation.hpp included successfully");
}
