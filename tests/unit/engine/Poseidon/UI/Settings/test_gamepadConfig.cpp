// GamepadConfig — gamepad scalar settings persistence.
// Same pattern coverage as AudioConfig / MouseConfig / DisplayConfig:
// defaults, fresh-instance equality with LoadDefaults, Normalize
// clamps, missing-file handling (returns false + leaves untouched),
// partial-file forward-compat documentation, full round-trip.

#include <catch2/catch_test_macros.hpp>

#include <Poseidon/UI/Settings/GamepadConfig.hpp>

#include <filesystem>
#include <random>
#include <string>
#include <catch2/catch_message.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>

using namespace Poseidon;
namespace
{
std::string TmpPath(const char* leaf)
{
    static std::random_device rd;
    static std::mt19937 rng(rd());
    std::uniform_int_distribution<unsigned> dist;
    auto root = std::filesystem::temp_directory_path() / ("gamepadcfg_test_" + std::to_string(dist(rng)));
    std::filesystem::create_directories(root);
    return (root / leaf).string();
}
} // namespace

TEST_CASE("GamepadConfig: factory defaults match GamepadState built-ins", "[Settings][GamepadConfig]")
{
    GamepadConfig c;
    CHECK(c.enabled == true);
    CHECK(c.deadzoneStick == 0.21f);
    CHECK(c.deadzoneTrigger == 0.10f);
    CHECK(c.lookSensitivity == 1.0f);
}

TEST_CASE("GamepadConfig: a fresh instance starts at LoadDefaults state", "[Settings][GamepadConfig]")
{
    GamepadConfig c;
    GamepadConfig defaulted;
    defaulted.LoadDefaults();
    CHECK(c.enabled == defaulted.enabled);
    CHECK(c.deadzoneStick == defaulted.deadzoneStick);
    CHECK(c.deadzoneTrigger == defaulted.deadzoneTrigger);
    CHECK(c.lookSensitivity == defaulted.lookSensitivity);
}

TEST_CASE("GamepadConfig: LoadDefaults resets a mutated instance", "[Settings][GamepadConfig]")
{
    GamepadConfig c;
    c.enabled = false;
    c.deadzoneStick = 0.4f;
    c.deadzoneTrigger = 0.4f;
    c.lookSensitivity = 3.0f;
    c.LoadDefaults();
    CHECK(c.enabled == true);
    CHECK(c.deadzoneStick == 0.21f);
    CHECK(c.deadzoneTrigger == 0.10f);
    CHECK(c.lookSensitivity == 1.0f);
}

TEST_CASE("GamepadConfig: Normalize clamps deadzones to [0.0, 0.5]", "[Settings][GamepadConfig]")
{
    GamepadConfig c;
    c.deadzoneStick = -0.1f;
    c.deadzoneTrigger = 0.9f;
    REQUIRE(c.Normalize());
    CHECK(c.deadzoneStick == 0.0f);
    CHECK(c.deadzoneTrigger == 0.5f);
}

TEST_CASE("GamepadConfig: Normalize clamps look sensitivity to [0.1, 5.0]", "[Settings][GamepadConfig]")
{
    GamepadConfig c;
    c.lookSensitivity = 0.0f;
    REQUIRE(c.Normalize());
    CHECK(c.lookSensitivity == 0.1f);

    c.lookSensitivity = 99.0f;
    REQUIRE(c.Normalize());
    CHECK(c.lookSensitivity == 5.0f);
}

TEST_CASE("GamepadConfig: Normalize is no-op for in-range values", "[Settings][GamepadConfig]")
{
    GamepadConfig c;
    c.deadzoneStick = 0.25f;
    c.deadzoneTrigger = 0.15f;
    c.lookSensitivity = 2.0f;
    CHECK_FALSE(c.Normalize());
    CHECK(c.deadzoneStick == 0.25f);
    CHECK(c.deadzoneTrigger == 0.15f);
    CHECK(c.lookSensitivity == 2.0f);
}

TEST_CASE("GamepadConfig: Load on missing file returns false", "[Settings][GamepadConfig]")
{
    GamepadConfig c;
    CHECK_FALSE(c.Load("does_not_exist_gamepad.cfg"));
}

TEST_CASE("GamepadConfig: Load on missing file leaves instance untouched", "[Settings][GamepadConfig]")
{
    GamepadConfig c;
    c.enabled = false;
    c.lookSensitivity = 2.5f;
    CHECK_FALSE(c.Load("nope_gamepad.cfg"));
    CHECK(c.enabled == false);
    CHECK(c.lookSensitivity == 2.5f);
}

TEST_CASE("GamepadConfig: Save then Load round-trips every scalar field", "[Settings][GamepadConfig]")
{
    const std::string path = TmpPath("gamepad_roundtrip.cfg");
    std::filesystem::remove(path);

    GamepadConfig src;
    src.enabled = false;
    src.deadzoneStick = 0.18f;
    src.deadzoneTrigger = 0.07f;
    src.lookSensitivity = 1.4f;
    REQUIRE(src.Save(path));

    GamepadConfig dst;
    REQUIRE(dst.Load(path));
    CHECK(dst.enabled == false);
    CHECK(dst.deadzoneStick == 0.18f);
    CHECK(dst.deadzoneTrigger == 0.07f);
    CHECK(dst.lookSensitivity == 1.4f);

    std::filesystem::remove(path);
}

TEST_CASE("GamepadConfig: IsGamepadCode classifies device bits correctly", "[Settings][GamepadConfig]")
{
    // STICK = 0x20000 / STICK_AXIS = 0x30000 / STICK_POV = 0x40000.
    CHECK(GamepadConfig::IsGamepadCode(0x20000 + 7));
    CHECK(GamepadConfig::IsGamepadCode(0x30000 + 0));
    CHECK(GamepadConfig::IsGamepadCode(0x40000 + 3));
    // Keyboard scancode (low bits) and mouse-button (0x10000) are NOT gamepad.
    CHECK_FALSE(GamepadConfig::IsGamepadCode(29));          // LCtrl
    CHECK_FALSE(GamepadConfig::IsGamepadCode(0x10000 + 1)); // mouse btn 1
    CHECK_FALSE(GamepadConfig::IsGamepadCode(-1));
}

TEST_CASE("GamepadConfig: Save ignores deprecated per-action bindings", "[Settings][GamepadConfig]")
{
    const std::string path = TmpPath("gamepad_filter.cfg");
    std::filesystem::remove(path);

    GamepadConfig src;
    src.bindings[UAFire].Add(0x20000 + 7);
    src.modifiers[UAFire].Add(-1);

    REQUIRE(src.Save(path));

    GamepadConfig dst;
    REQUIRE(dst.Load(path));
    CHECK(dst.bindings[UAFire].Size() == 0);
    CHECK(dst.modifiers[UAFire].Size() == 0);

    std::filesystem::remove(path);
}

TEST_CASE("GamepadConfig: LoadDefaults clears deprecated per-action bindings", "[Settings][GamepadConfig]")
{
    GamepadConfig c;
    c.bindings[UAFire].Add(0x20000 + 7);
    c.modifiers[UAFire].Add(-1);
    c.LoadDefaults();

    for (int i = 0; i < UAN; i++)
    {
        CHECK(c.bindings[i].Size() == 0);
        CHECK(c.modifiers[i].Size() == 0);
    }
}
