#include <catch2/catch_test_macros.hpp>

#include <Poseidon/UI/Settings/DisplayStartupOverrides.hpp>
#include <optional>

using Poseidon::DisplayStartupOverrideRequest;
using Poseidon::DisplayStartupOverrides;

using Poseidon::DisplayConfig;

TEST_CASE("Display startup overrides force windowed mode over persisted fullscreen",
          "[Settings][DisplayConfig][startup]")
{
    DisplayConfig cfg;
    cfg.windowMode = DisplayConfig::Fullscreen;
    cfg.resolutionWidth = 1920;
    cfg.resolutionHeight = 1080;

    DisplayStartupOverrides overrides;
    overrides.windowMode = DisplayConfig::Windowed;

    REQUIRE(ApplyDisplayStartupOverrides(cfg, overrides));
    CHECK(cfg.windowMode == DisplayConfig::Windowed);
    CHECK(cfg.resolutionWidth == 1920);
    CHECK(cfg.resolutionHeight == 1080);
}

TEST_CASE("Display startup overrides replace persisted size only for explicit CLI dimensions",
          "[Settings][DisplayConfig][startup]")
{
    DisplayConfig cfg;
    cfg.windowMode = DisplayConfig::Borderless;
    cfg.resolutionWidth = 2560;
    cfg.resolutionHeight = 1440;

    DisplayStartupOverrides overrides;
    overrides.windowMode = DisplayConfig::Windowed;
    overrides.resolutionWidth = 1280;

    REQUIRE(ApplyDisplayStartupOverrides(cfg, overrides));
    CHECK(cfg.windowMode == DisplayConfig::Windowed);
    CHECK(cfg.resolutionWidth == 1280);
    CHECK(cfg.resolutionHeight == 1440);
}

TEST_CASE("Display startup overrides are a no-op when CLI did not set display flags",
          "[Settings][DisplayConfig][startup]")
{
    DisplayConfig cfg;
    cfg.windowMode = DisplayConfig::Borderless;
    cfg.resolutionWidth = 0;
    cfg.resolutionHeight = 0;

    DisplayStartupOverrides overrides;

    CHECK_FALSE(ApplyDisplayStartupOverrides(cfg, overrides));
    CHECK(cfg.windowMode == DisplayConfig::Borderless);
    CHECK(cfg.resolutionWidth == 0);
    CHECK(cfg.resolutionHeight == 0);
}

TEST_CASE("Display startup override request gives --window a default 800x600 size",
          "[Settings][DisplayConfig][startup]")
{
    DisplayStartupOverrideRequest request;
    request.windowFlagExplicit = true;
    request.windowMode = DisplayConfig::Windowed;

    DisplayStartupOverrides overrides = BuildDisplayStartupOverrides(request);

    REQUIRE(overrides.windowMode == DisplayConfig::Windowed);
    REQUIRE(overrides.resolutionWidth == 800);
    REQUIRE(overrides.resolutionHeight == 600);
}

TEST_CASE("Display startup override request keeps explicit size over --window defaults",
          "[Settings][DisplayConfig][startup]")
{
    DisplayStartupOverrideRequest request;
    request.windowFlagExplicit = true;
    request.windowMode = DisplayConfig::Windowed;
    request.resolutionWidth = 1280;

    DisplayStartupOverrides overrides = BuildDisplayStartupOverrides(request);

    REQUIRE(overrides.resolutionWidth == 1280);
    REQUIRE(overrides.resolutionHeight == 600);
}
