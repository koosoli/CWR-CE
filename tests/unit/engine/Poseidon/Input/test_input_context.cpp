#include <Poseidon/Input/InputContext.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Poseidon;
TEST_CASE("InputContext enum values are distinct", "[input][InputContext]")
{
    REQUIRE(static_cast<int>(InputContext::Menu) != static_cast<int>(InputContext::Infantry));
    REQUIRE(static_cast<int>(InputContext::Infantry) != static_cast<int>(InputContext::CarDriver));
    REQUIRE(static_cast<int>(InputContext::CarDriver) != static_cast<int>(InputContext::TankDriver));
    REQUIRE(static_cast<int>(InputContext::HeliPilot) != static_cast<int>(InputContext::PlanePilot));
    REQUIRE(static_cast<int>(InputContext::Map) != static_cast<int>(InputContext::Chat));
    REQUIRE(static_cast<int>(InputContext::Editor) != static_cast<int>(InputContext::Spectator));
}

TEST_CASE("InputContext has expected vehicle types", "[input][InputContext]")
{
    InputContext contexts[] = {
        InputContext::Infantry,  InputContext::CarDriver,  InputContext::TankDriver,
        InputContext::HeliPilot, InputContext::PlanePilot, InputContext::ShipDriver,
    };
    // All distinct
    for (int i = 0; i < 6; i++)
        for (int j = i + 1; j < 6; j++)
            REQUIRE(static_cast<int>(contexts[i]) != static_cast<int>(contexts[j]));
}
