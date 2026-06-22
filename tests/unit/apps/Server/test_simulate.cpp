// test_simulate.cpp - Tests for --simulate mode internals
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Platform/AppConfig.hpp>
#include <Poseidon/Foundation/Logging/Logging.hpp>
#include <string>

TEST_CASE("AppConfig simulate flag defaults", "[simulate]")
{
    auto& config = AppConfig::Instance();
    REQUIRE(config.IsSimulateMode() == false);
    REQUIRE(config.GetSimulateDuration() == 0);
}

TEST_CASE("Simulate mode flag is accessible for player guard", "[simulate][no-player]")
{
    auto& config = AppConfig::Instance();
    bool simulateMode = config.IsSimulateMode();
    REQUIRE(simulateMode == false);
    bool playerGateActive = !simulateMode;
    REQUIRE(playerGateActive == true);
}

TEST_CASE("Test mission path is accessible for SimulateDS injection", "[simulate][mission-load]")
{
    auto& config = AppConfig::Instance();
    // Default: no test mission path
    REQUIRE(config.GetTestMissionPath().empty());
    // In simulate mode, the path would be injected into _mission
    REQUIRE(config.IsSimulateMode() == false);
}

TEST_CASE("Mission logging category exists", "[simulate]")
{
    auto name = Poseidon::Foundation::LoggingSystem::GetCategoryName(Poseidon::Foundation::LoggingSystem::Category::Mission);
    REQUIRE(std::string(name) == "MISSION");
}

TEST_CASE("Mission category color is non-null", "[simulate]")
{
    auto color = Poseidon::Foundation::LoggingSystem::GetCategoryColor(Poseidon::Foundation::LoggingSystem::Category::Mission);
    REQUIRE(color != nullptr);
    REQUIRE(color[0] == '\033');
}

TEST_CASE("Mission category tag is formatted", "[simulate]")
{
    auto tag = Poseidon::Foundation::LoggingSystem::GetColoredCategoryTag(Poseidon::Foundation::LoggingSystem::Category::Mission);
    REQUIRE(tag != nullptr);
    REQUIRE(std::string(tag).find("MISSION") != std::string::npos);
}
