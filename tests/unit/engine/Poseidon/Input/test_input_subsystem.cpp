#include <Poseidon/Input/InputCode.hpp>
#include <Poseidon/Input/InputBinding.hpp>
#include <Poseidon/Input/UserAction.hpp>
#include <SDL3/SDL_scancode.h>
#include <catch2/catch_test_macros.hpp>
#include <stdint.h>

using namespace Poseidon;
TEST_CASE("InputCode factory methods produce correct values", "[input][InputCode]")
{
    auto key = InputCode::Key(SDL_SCANCODE_W);
    REQUIRE(key.device() == InputDevice::Keyboard);
    REQUIRE(key.code() == SDL_SCANCODE_W);
    REQUIRE(key.valid());

    auto mouse = InputCode::MouseButton(1);
    REQUIRE(mouse.device() == InputDevice::Mouse);
    REQUIRE(mouse.code() == 1);

    auto btn = InputCode::GamepadBtn(3);
    REQUIRE(btn.device() == InputDevice::GamepadButton);
    REQUIRE(btn.code() == 3);

    auto axis = InputCode::GamepadAx(2);
    REQUIRE(axis.device() == InputDevice::GamepadAxis);
    REQUIRE(axis.code() == 2);

    auto pov = InputCode::GamepadPov(4);
    REQUIRE(pov.device() == InputDevice::GamepadPOV);
    REQUIRE(pov.code() == 4);
}

TEST_CASE("InputCode default is invalid", "[input][InputCode]")
{
    InputCode empty;
    REQUIRE(!empty.valid());
    REQUIRE(empty.raw == 0);
}

TEST_CASE("InputCode equality and comparison", "[input][InputCode]")
{
    auto a = InputCode::Key(SDL_SCANCODE_W);
    auto b = InputCode::Key(SDL_SCANCODE_W);
    auto c = InputCode::Key(SDL_SCANCODE_S);
    auto d = InputCode::MouseButton(0);

    REQUIRE(a == b);
    REQUIRE(a != c);
    REQUIRE(a != d);
}

TEST_CASE("InputCode legacy round-trip", "[input][InputCode]")
{
    int legacy = 0x00010001; // INPUT_DEVICE_MOUSE | 1
    auto code = InputCode::FromLegacy(legacy);
    REQUIRE(code.device() == InputDevice::Mouse);
    REQUIRE(code.code() == 1);
    REQUIRE(code.toLegacy() == legacy);

    int legacyKey = static_cast<int>(SDL_SCANCODE_ESCAPE);
    auto keyCode = InputCode::FromLegacy(legacyKey);
    REQUIRE(keyCode.device() == InputDevice::Keyboard);
    REQUIRE(keyCode.code() == SDL_SCANCODE_ESCAPE);
}

TEST_CASE("InputCode devices are distinct", "[input][InputCode]")
{
    auto key = InputCode::Key(SDL_SCANCODE_A);
    auto mouse = InputCode::MouseButton(0);
    auto btn = InputCode::GamepadBtn(0);
    auto axis = InputCode::GamepadAx(0);
    auto pov = InputCode::GamepadPov(0);

    REQUIRE(key != mouse);
    REQUIRE(mouse != btn);
    REQUIRE(btn != axis);
    REQUIRE(axis != pov);
}

TEST_CASE("InputBinding default mode is OnHold", "[input][InputBinding]")
{
    InputBinding binding(InputCode::Key(SDL_SCANCODE_W));
    REQUIRE(binding.mode == ActivationMode::OnHold);
    REQUIRE(binding.code == InputCode::Key(SDL_SCANCODE_W));
}

TEST_CASE("InputBinding modes", "[input][InputBinding]")
{
    InputBinding press(InputCode::Key(SDL_SCANCODE_R), ActivationMode::OnPress);
    REQUIRE(press.mode == ActivationMode::OnPress);

    InputBinding release(InputCode::MouseButton(0), ActivationMode::OnRelease);
    REQUIRE(release.mode == ActivationMode::OnRelease);
}

TEST_CASE("UserAction enum values are sequential", "[input][UserAction]")
{
    REQUIRE(UAMoveForward == 0);
    REQUIRE(UAMoveBack == 1);
    REQUIRE(UATurnLeft == 2);
    REQUIRE(UATurnRight == 3);
    REQUIRE(UAN > 0);
}

TEST_CASE("InputDevice values match legacy INPUT_DEVICE constants", "[input][InputCode]")
{
    REQUIRE(static_cast<uint32_t>(InputDevice::Keyboard) == 0x00000000u);
    REQUIRE(static_cast<uint32_t>(InputDevice::Mouse) == 0x00010000u);
    REQUIRE(static_cast<uint32_t>(InputDevice::GamepadButton) == 0x00020000u);
    REQUIRE(static_cast<uint32_t>(InputDevice::GamepadAxis) == 0x00030000u);
    REQUIRE(static_cast<uint32_t>(InputDevice::GamepadPOV) == 0x00040000u);
}
