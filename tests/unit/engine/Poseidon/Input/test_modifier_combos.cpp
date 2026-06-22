// Modifier-combo support — v1 storage extension that lets bindings
// carry an optional modifier scancode in the parallel
// userKeysModifiers[] array.  Tests:
//   - FindBindingConflict treats profile (code, mod) as a pair:
//     "Ctrl+W" doesn't conflict with bare "W".
//   - QueryAction (and ActionToDo) gates on modifier-held when set.
//   - Round-trip through ControlsConfig save/load preserves modifiers.
//   - LoadDefaults seeds modifiers to -1 per binding.
//   - Reset / SaveKeys reset GInput modifiers in lock-step with bindings.

#include <Poseidon/Input/InputSubsystem.hpp>
#include <Poseidon/Input/InputBinding.hpp>
#include <Poseidon/Input/InputProfile.hpp>
#include <Poseidon/Input/KeyInput.hpp>
#include <Poseidon/UI/Settings/ControlsConfig.hpp>
#include <SDL3/SDL_scancode.h>
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <random>
#include <string>
#include <catch2/catch_message.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>

using namespace Poseidon;
namespace Poseidon
{
extern Input GInput;
}

namespace
{
class UserKeysSnapshot
{
  public:
    UserKeysSnapshot()
        : savedContext(InputSubsystem::Instance().GetContext()),
          savedMenuProfile(InputSubsystem::Instance().GetProfile(InputContext::Menu)),
          savedInfantryProfile(InputSubsystem::Instance().GetProfile(InputContext::Infantry))
    {
        for (int i = 0; i < UAN; i++)
        {
            savedKeys[i] = GInput.userKeys[i];
            savedMods[i] = GInput.userKeysModifiers[i];
        }
    }
    ~UserKeysSnapshot()
    {
        auto& sub = InputSubsystem::Instance();
        sub.SetContext(savedContext);
        sub.GetProfile(InputContext::Menu) = savedMenuProfile;
        sub.GetProfile(InputContext::Infantry) = savedInfantryProfile;
        for (int i = 0; i < UAN; i++)
        {
            GInput.userKeys[i] = savedKeys[i];
            GInput.userKeysModifiers[i] = savedMods[i];
        }
    }

  private:
    AutoArray<int> savedKeys[UAN];
    AutoArray<int> savedMods[UAN];
    InputContext savedContext;
    InputProfile savedMenuProfile;
    InputProfile savedInfantryProfile;
};

class KeyboardSnapshot
{
  public:
    KeyboardSnapshot()
    {
        for (int i = 0; i < SDL_SCANCODE_COUNT; i++)
            saved[i] = GInput.keyboard.keys[i];
    }
    ~KeyboardSnapshot()
    {
        for (int i = 0; i < SDL_SCANCODE_COUNT; i++)
            GInput.keyboard.keys[i] = saved[i];
    }

  private:
    float saved[SDL_SCANCODE_COUNT];
};

std::string TmpPath(const char* leaf)
{
    static std::random_device rd;
    static std::mt19937 rng(rd());
    std::uniform_int_distribution<unsigned> dist;
    auto root = std::filesystem::temp_directory_path() / ("controls_mod_test_" + std::to_string(dist(rng)));
    std::filesystem::create_directories(root);
    return (root / leaf).string();
}

void BindMenuProfile(UserAction action, int code, int modifier)
{
    auto& sub = InputSubsystem::Instance();
    sub.SetContext(InputContext::Menu);
    auto& profile = sub.GetProfile(InputContext::Menu);
    profile.ClearBindings(action);
    InputCode modCode = modifier >= 0 ? InputCode::FromLegacy(modifier) : InputCode{};
    profile.Bind(action, InputBinding(InputCode::FromLegacy(code), modCode));
}
} // namespace

TEST_CASE("FindBindingConflict: Ctrl+W and bare W are distinct bindings", "[Input][ModifierCombos]")
{
    UserKeysSnapshot snap;
    auto& sub = InputSubsystem::Instance();

    sub.SetContext(InputContext::Menu);
    sub.GetProfile(InputContext::Menu).ClearAll();
    BindMenuProfile(UAFire, (int)SDL_SCANCODE_W, (int)SDL_SCANCODE_LCTRL);

    // Bare W should NOT conflict (modifier mismatch).
    CHECK(sub.FindBindingConflict((int)SDL_SCANCODE_W, UAN, -1, -1) == UAN);
    // Ctrl+W should conflict (both match).
    CHECK(sub.FindBindingConflict((int)SDL_SCANCODE_W, UAN, -1, (int)SDL_SCANCODE_LCTRL) == UAFire);
    // Shift+W should NOT conflict (different modifier).
    CHECK(sub.FindBindingConflict((int)SDL_SCANCODE_W, UAN, -1, (int)SDL_SCANCODE_LSHIFT) == UAN);
}

TEST_CASE("QueryAction: bare binding fires, modifier binding requires modifier-held", "[Input][ModifierCombos]")
{
    UserKeysSnapshot ksnap;
    KeyboardSnapshot kbsnap;
    auto& sub = InputSubsystem::Instance();

    for (int i = 0; i < UAN; i++)
    {
        GInput.userKeys[i].Resize(0);
        GInput.userKeysModifiers[i].Resize(0);
    }

    // Bind UAFire → Ctrl+W.
    GInput.userKeys[UAFire].Add((int)SDL_SCANCODE_W);
    GInput.userKeysModifiers[UAFire].Add((int)SDL_SCANCODE_LCTRL);
    BindMenuProfile(UAFire, (int)SDL_SCANCODE_W, (int)SDL_SCANCODE_LCTRL);

    // Reset keyboard state.
    for (int i = 0; i < SDL_SCANCODE_COUNT; i++)
        GInput.keyboard.keys[i] = 0;
    GInput.gameFocusLost = 0;

    // W held without Ctrl — modifier requirement fails.
    GInput.keyboard.keys[SDL_SCANCODE_W] = 1.0f;
    CHECK(sub.GetAction(UAFire, false) == 0.0f);

    // W + Ctrl held — fires.
    GInput.keyboard.keys[SDL_SCANCODE_LCTRL] = 1.0f;
    CHECK(sub.GetAction(UAFire, false) > 0.0f);

    // Ctrl alone — main key not held, no fire.
    GInput.keyboard.keys[SDL_SCANCODE_W] = 0;
    CHECK(sub.GetAction(UAFire, false) == 0.0f);
}

TEST_CASE("QueryAction: bare binding ignores modifier state", "[Input][ModifierCombos]")
{
    UserKeysSnapshot ksnap;
    KeyboardSnapshot kbsnap;
    auto& sub = InputSubsystem::Instance();

    for (int i = 0; i < UAN; i++)
    {
        GInput.userKeys[i].Resize(0);
        GInput.userKeysModifiers[i].Resize(0);
    }
    GInput.userKeys[UAMoveForward].Add((int)SDL_SCANCODE_W);
    GInput.userKeysModifiers[UAMoveForward].Add(-1); // no modifier
    BindMenuProfile(UAMoveForward, (int)SDL_SCANCODE_W, -1);

    for (int i = 0; i < SDL_SCANCODE_COUNT; i++)
        GInput.keyboard.keys[i] = 0;
    GInput.gameFocusLost = 0;

    // W alone — fires.
    GInput.keyboard.keys[SDL_SCANCODE_W] = 1.0f;
    CHECK(sub.GetAction(UAMoveForward, false) > 0.0f);

    // W + Ctrl — also fires (bare binding doesn't care about Ctrl).
    GInput.keyboard.keys[SDL_SCANCODE_LCTRL] = 1.0f;
    CHECK(sub.GetAction(UAMoveForward, false) > 0.0f);
}

TEST_CASE("ControlsConfig: round-trips modifier list", "[Settings][ControlsConfig][ModifierCombos]")
{
    const std::string path = TmpPath("mods_roundtrip.cfg");
    std::filesystem::remove(path);

    ControlsConfig src;
    src.LoadDefaults();
    src.bindings[UAFire].Resize(0);
    src.bindings[UAFire].Add((int)SDL_SCANCODE_W);
    src.bindings[UAFire].Add((int)SDL_SCANCODE_J);
    src.modifiers[UAFire].Resize(0);
    src.modifiers[UAFire].Add((int)SDL_SCANCODE_LCTRL);
    src.modifiers[UAFire].Add(-1);

    REQUIRE(src.Save(path));

    ControlsConfig dst;
    REQUIRE(dst.Load(path));
    CHECK(dst.bindings[UAFire].Size() == 2);
    CHECK(dst.bindings[UAFire][0] == (int)SDL_SCANCODE_W);
    CHECK(dst.bindings[UAFire][1] == (int)SDL_SCANCODE_J);
    CHECK(dst.modifiers[UAFire].Size() == 2);
    CHECK(dst.modifiers[UAFire][0] == (int)SDL_SCANCODE_LCTRL);
    CHECK(dst.modifiers[UAFire][1] == -1);

    std::filesystem::remove(path);
}

TEST_CASE("ControlsConfig: LoadDefaults seeds modifiers to -1 in lockstep with bindings",
          "[Settings][ControlsConfig][ModifierCombos]")
{
    ControlsConfig c;
    c.LoadDefaults();
    for (int i = 0; i < UAN; i++)
    {
        CAPTURE(i);
        REQUIRE(c.bindings[i].Size() == c.modifiers[i].Size());
        for (int j = 0; j < c.modifiers[i].Size(); j++)
            CHECK(c.modifiers[i][j] == -1);
    }
}

TEST_CASE("ControlsConfig: legacy file (no _mod lines) loads with -1 modifiers",
          "[Settings][ControlsConfig][ModifierCombos]")
{
    const std::string path = TmpPath("legacy_no_mod.cfg");
    std::filesystem::remove(path);

    // Manually craft a pre-modifier config that only writes key<Action>[]
    // arrays, simulating any older saved state.
    ControlsConfig src;
    src.LoadDefaults();
    src.bindings[UAFire].Resize(0);
    src.bindings[UAFire].Add(99);
    // modifiers[UAFire] kept at default-from-LoadDefaults — all -1, same length
    REQUIRE(src.Save(path));

    ControlsConfig dst;
    REQUIRE(dst.Load(path));
    CHECK(dst.bindings[UAFire].Size() == 1);
    CHECK(dst.bindings[UAFire][0] == 99);
    REQUIRE(dst.modifiers[UAFire].Size() == 1);
    CHECK(dst.modifiers[UAFire][0] == -1);

    std::filesystem::remove(path);
}

TEST_CASE("ResetCategoryDefaults clears modifiers to -1 in lockstep", "[Input][ModifierCombos]")
{
    UserKeysSnapshot snap;
    auto& sub = InputSubsystem::Instance();

    auto& infantry = sub.GetProfile(InputContext::Infantry);
    infantry.ClearBindings(UAMoveForward);
    infantry.Bind(UAMoveForward, InputBinding(InputCode::Key(SDL_SCANCODE_W), InputCode::Key(SDL_SCANCODE_LCTRL)));

    sub.ResetCategoryDefaults(ControlsCategoryOnFoot);

    const auto& bindings = infantry.GetBindingEntries(UAMoveForward);
    REQUIRE_FALSE(bindings.empty());
    for (const InputBinding& binding : bindings)
        CHECK_FALSE(binding.modifier.valid());
}
