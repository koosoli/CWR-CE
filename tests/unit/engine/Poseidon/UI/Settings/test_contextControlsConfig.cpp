#include <Poseidon/UI/Settings/ContextControlsConfig.hpp>

#include <Poseidon/Input/InputBinding.hpp>
#include <Poseidon/Input/InputCode.hpp>
#include <Poseidon/Input/UserAction.hpp>
#include <SDL3/SDL_scancode.h>
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <random>
#include <string>

using namespace Poseidon;

namespace
{
std::string TmpPath(const char* leaf)
{
    static std::random_device rd;
    static std::mt19937 rng(rd());
    std::uniform_int_distribution<unsigned> dist;
    auto root = std::filesystem::temp_directory_path() / ("context_controls_test_" + std::to_string(dist(rng)));
    std::filesystem::create_directories(root);
    return (root / leaf).string();
}
} // namespace

TEST_CASE("ContextControlsConfig: missing file returns false", "[Settings][ContextControlsConfig]")
{
    ContextControlsConfig cfg;
    CHECK_FALSE(cfg.Load(TmpPath("missing.cfg")));
}

TEST_CASE("ContextControlsConfig: Save then Load round-trips separate context profiles",
          "[Settings][ContextControlsConfig]")
{
    const std::string path = TmpPath("context_controls.cfg");
    std::filesystem::remove(path);

    ContextControlsConfig src;
    src.profiles[(int)InputContext::Infantry].Bind(
        UAMoveForward, InputBinding(InputCode::GamepadAx(1), {}, ActivationMode::OnHold, -1.0f));
    src.profiles[(int)InputContext::CarDriver].Bind(
        UAMoveForward, InputBinding(InputCode::GamepadAx(2), InputCode::GamepadBtn(6), ActivationMode::OnHold, 0.5f));
    src.profiles[(int)InputContext::Infantry].Bind(UAFire, InputCode::Key(SDL_SCANCODE_SPACE));

    REQUIRE(src.Save(path));

    ContextControlsConfig dst;
    REQUIRE(dst.Load(path));

    const auto& infantryMove = dst.profiles[(int)InputContext::Infantry].GetBindingEntries(UAMoveForward);
    REQUIRE(infantryMove.size() == 1);
    CHECK(infantryMove[0].code == InputCode::GamepadAx(1));
    CHECK_FALSE(infantryMove[0].modifier.valid());
    CHECK(infantryMove[0].scale == -1.0f);

    const auto& carMove = dst.profiles[(int)InputContext::CarDriver].GetBindingEntries(UAMoveForward);
    REQUIRE(carMove.size() == 1);
    CHECK(carMove[0].code == InputCode::GamepadAx(2));
    CHECK(carMove[0].modifier == InputCode::GamepadBtn(6));
    CHECK(carMove[0].scale == 0.5f);

    CHECK(dst.profiles[(int)InputContext::Infantry].HasBinding(UAFire, InputCode::Key(SDL_SCANCODE_SPACE)));
    CHECK_FALSE(dst.profiles[(int)InputContext::CarDriver].HasBinding(UAFire, InputCode::Key(SDL_SCANCODE_SPACE)));

    std::filesystem::remove(path);
}
