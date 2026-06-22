// InputSubsystem::FindBindingConflict + ResetCategoryDefaults - engine
// helpers used by the Controls UI. Tests use the real singleton with
// state snapshotted/restored.

#include <Poseidon/Input/InputSubsystem.hpp>
#include <Poseidon/Input/InputCode.hpp>
#include <Poseidon/Input/InputContext.hpp>
#include <Poseidon/Input/InputDeviceConstants.hpp>
#include <Poseidon/Input/InputProfile.hpp>
#include <Poseidon/Input/UserActionDesc.hpp>
#include <Poseidon/Input/KeyInput.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <SDL3/SDL_scancode.h>

using namespace Poseidon;
namespace Poseidon
{
extern Input GInput;
}

namespace
{
bool IsGamepadCode(int packedCode)
{
    const int dev = packedCode & INPUT_DEVICE_MASK;
    return dev == INPUT_DEVICE_STICK || dev == INPUT_DEVICE_STICK_AXIS || dev == INPUT_DEVICE_STICK_POV;
}

class UserKeysSnapshot
{
  public:
    UserKeysSnapshot()
        : savedContext(InputSubsystem::Instance().GetContext()),
          savedInfantryProfile(InputSubsystem::Instance().GetProfile(InputContext::Infantry))
    {
        for (int i = 0; i < UAN; i++)
        {
            saved[i] = GInput.userKeys[i];
            savedMods[i] = GInput.userKeysModifiers[i];
        }
    }
    ~UserKeysSnapshot()
    {
        auto& sub = InputSubsystem::Instance();
        sub.SetContext(savedContext);
        sub.GetProfile(InputContext::Infantry) = savedInfantryProfile;
        for (int i = 0; i < UAN; i++)
        {
            GInput.userKeys[i] = saved[i];
            GInput.userKeysModifiers[i] = savedMods[i];
        }
    }

  private:
    InputContext savedContext;
    InputProfile savedInfantryProfile;
    AutoArray<int> saved[UAN];
    AutoArray<int> savedMods[UAN];
};

void ClearAllBindings()
{
    auto& sub = InputSubsystem::Instance();
    sub.SetContext(InputContext::Infantry);
    sub.GetProfile(InputContext::Infantry).ClearAll();
}

void SetSingleBinding(UserAction a, int code)
{
    InputProfile& profile = InputSubsystem::Instance().GetProfile(InputContext::Infantry);
    profile.ClearBindings(a);
    profile.Bind(a, InputCode::FromLegacy(code));
}
} // namespace

TEST_CASE("FindBindingConflict: missing code returns UAN", "[Input][BindingConflict]")
{
    UserKeysSnapshot snap;
    auto& sub = InputSubsystem::Instance();

    ClearAllBindings();
    CHECK(sub.FindBindingConflict(0xDEADBEEF) == UAN);
}

TEST_CASE("FindBindingConflict: finds the action with a matching binding", "[Input][BindingConflict]")
{
    UserKeysSnapshot snap;
    auto& sub = InputSubsystem::Instance();

    ClearAllBindings();
    SetSingleBinding(UAFire, 1234);

    CHECK(sub.FindBindingConflict(1234) == UAFire);
    CHECK(sub.FindBindingConflict(9999) == UAN);
}

TEST_CASE("FindBindingConflict: excludeAction skips the entire action when slot=-1", "[Input][BindingConflict]")
{
    UserKeysSnapshot snap;
    auto& sub = InputSubsystem::Instance();

    ClearAllBindings();
    SetSingleBinding(UAFire, 5555);
    SetSingleBinding(UAReloadMagazine, 5555);

    CHECK(sub.FindBindingConflict(5555, UAFire, -1) == UAReloadMagazine);
    CHECK(sub.FindBindingConflict(5555) == UAFire);
}

TEST_CASE("FindBindingConflict: excludeSlot skips only that slot of excludeAction", "[Input][BindingConflict]")
{
    UserKeysSnapshot snap;
    auto& sub = InputSubsystem::Instance();

    ClearAllBindings();

    InputProfile& profile = sub.GetProfile(InputContext::Infantry);
    profile.Bind(UAFire, InputBinding(InputCode::FromLegacy(7777), InputCode::Key(SDL_SCANCODE_LCTRL)));
    profile.Bind(UAFire, InputCode::FromLegacy(7777));

    CHECK(sub.FindBindingConflict(7777, UAFire, 0) == UAFire);

    profile.ClearBindings(UAFire);
    profile.Bind(UAFire, InputBinding(InputCode::FromLegacy(7777), InputCode::Key(SDL_SCANCODE_LCTRL)));
    CHECK(sub.FindBindingConflict(7777, UAFire, 0) == UAN);
}

TEST_CASE("FindBindingConflict: negative packed code is rejected", "[Input][BindingConflict]")
{
    UserKeysSnapshot snap;
    auto& sub = InputSubsystem::Instance();
    CHECK(sub.FindBindingConflict(-1) == UAN);
    CHECK(sub.FindBindingConflict(-999) == UAN);
}

TEST_CASE("FindBindingConflict: double-tap and single key are distinct bindings", "[Input][BindingConflict]")
{
    UserKeysSnapshot snap;
    auto& sub = InputSubsystem::Instance();

    ClearAllBindings();
    SetSingleBinding(UAFire, (int)SDL_SCANCODE_G);

    CHECK(sub.FindBindingConflict(InputBindingDoubleTapCode((int)SDL_SCANCODE_G)) == UAN);
    CHECK(sub.FindBindingConflict((int)SDL_SCANCODE_G) == UAFire);
}

TEST_CASE("ResetCategoryDefaults: rewrites every action in the category", "[Input][ResetCategory]")
{
    UserKeysSnapshot snap;
    auto& sub = InputSubsystem::Instance();
    UserActionDesc* descs = InputSubsystem::GetUserActionDesc();

    ClearAllBindings();
    auto& profile = sub.GetProfile(InputContext::Infantry);
    profile.ClearAll();

    sub.ResetCategoryDefaults(ControlsCategoryOnFoot);

    const UserAction* list = GetControlsCategoryActions(ControlsCategoryOnFoot);
    for (int i = 0; list[i] != UAN; i++)
    {
        int idx = (int)list[i];
        CAPTURE(idx);
        int expected = 0;
        for (int j = 0; j < descs[idx].keys.Size(); j++)
        {
            if (!IsGamepadCode(descs[idx].keys[j]))
                expected++;
        }
        const auto& bindings = profile.GetBindingEntries(static_cast<UserAction>(idx));
        REQUIRE(static_cast<int>(bindings.size()) == expected);
        int actual = 0;
        for (int j = 0; j < descs[idx].keys.Size(); j++)
        {
            if (IsGamepadCode(descs[idx].keys[j]))
                continue;
            CHECK(bindings[actual].code == InputCode::FromLegacy(descs[idx].keys[j]));
            CHECK_FALSE(bindings[actual].modifier.valid());
            actual++;
        }
    }
}

TEST_CASE("ResetCategoryDefaults: leaves out-of-category actions untouched", "[Input][ResetCategory]")
{
    UserKeysSnapshot snap;
    auto& sub = InputSubsystem::Instance();

    auto& profile = sub.GetProfile(InputContext::Infantry);
    profile.ClearBindings(UAMap);
    profile.Bind(UAMap, InputCode::FromLegacy(0xA5A5A5));

    sub.ResetCategoryDefaults(ControlsCategoryOnFoot);

    REQUIRE(profile.BindingCount(UAMap) == 1);
    CHECK(profile.GetBindingEntries(UAMap)[0].code == InputCode::FromLegacy(0xA5A5A5));
}
