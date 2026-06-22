// End-to-end regression: bind UAFire to Ctrl+C via the same code path
// the in-game settings UI uses (BindingsPage::ApplyCapture-equivalent
// writes through to GInput.userKeys / userKeysModifiers), then verify
// the gameplay-side action resolver fires when Ctrl+C is pressed.
//
// This is the "press it in-game and ammo goes down" cycle reduced to
// the engine logic — InfantryController calls input.GetActionToDo(UAFire)
// each frame; if that returns true, the soldier's PilotOutput.fire flag
// is set and the fire-control pipeline consumes a round.  We verify the
// GetActionToDo edge here, since per-frame InfantryController execution
// requires a live mission which the Trident test covers separately.

#include <Poseidon/Input/InputSubsystem.hpp>
#include <Poseidon/Input/InputBinding.hpp>
#include <Poseidon/Input/InputDeviceConstants.hpp>
#include <Poseidon/Input/InputProfile.hpp>
#include <Poseidon/Input/KeyInput.hpp>
#include <SDL3/SDL_scancode.h>
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>

using namespace Poseidon;
namespace Poseidon
{
extern Input GInput;
}

namespace
{
class FullStateSnapshot
{
  public:
    FullStateSnapshot()
        : savedContext(InputSubsystem::Instance().GetContext()),
          savedMenuProfile(InputSubsystem::Instance().GetProfile(InputContext::Menu))
    {
        for (int i = 0; i < UAN; i++)
        {
            savedKeys[i] = GInput.userKeys[i];
            savedMods[i] = GInput.userKeysModifiers[i];
            savedActionDone[i] = GInput.actionDone[i];
        }
        for (int i = 0; i < SDL_SCANCODE_COUNT; i++)
        {
            savedKb[i] = GInput.keyboard.keys[i];
            savedKbToDo[i] = GInput.keyboard.keysToDo[i];
            savedKbDoubleToDo[i] = GInput.keyboard.keysDoubleTapToDo[i];
            savedKbDoubleActive[i] = GInput.keyboard.keysDoubleTapActive[i];
        }
        for (int i = 0; i < N_MOUSE_BUTTONS; i++)
        {
            savedMouseButtons[i] = GInput.mouse.buttons[i];
            savedMouseToDo[i] = GInput.mouse.buttonsToDo[i];
            savedMouseDoubleToDo[i] = GInput.mouse.buttonsDoubleToDo[i];
            savedMouseDoubleActive[i] = GInput.mouse.buttonsDoubleActive[i];
        }
        savedFocus = GInput.gameFocusLost;
    }
    ~FullStateSnapshot()
    {
        auto& sub = InputSubsystem::Instance();
        sub.SetContext(savedContext);
        sub.GetProfile(InputContext::Menu) = savedMenuProfile;
        for (int i = 0; i < UAN; i++)
        {
            GInput.userKeys[i] = savedKeys[i];
            GInput.userKeysModifiers[i] = savedMods[i];
            GInput.actionDone[i] = savedActionDone[i];
        }
        for (int i = 0; i < SDL_SCANCODE_COUNT; i++)
        {
            GInput.keyboard.keys[i] = savedKb[i];
            GInput.keyboard.keysToDo[i] = savedKbToDo[i];
            GInput.keyboard.keysDoubleTapToDo[i] = savedKbDoubleToDo[i];
            GInput.keyboard.keysDoubleTapActive[i] = savedKbDoubleActive[i];
        }
        for (int i = 0; i < N_MOUSE_BUTTONS; i++)
        {
            GInput.mouse.buttons[i] = savedMouseButtons[i];
            GInput.mouse.buttonsToDo[i] = savedMouseToDo[i];
            GInput.mouse.buttonsDoubleToDo[i] = savedMouseDoubleToDo[i];
            GInput.mouse.buttonsDoubleActive[i] = savedMouseDoubleActive[i];
        }
        GInput.gameFocusLost = savedFocus;
    }

  private:
    AutoArray<int> savedKeys[UAN];
    AutoArray<int> savedMods[UAN];
    InputContext savedContext;
    InputProfile savedMenuProfile;
    bool savedActionDone[UAN];
    float savedKb[SDL_SCANCODE_COUNT];
    bool savedKbToDo[SDL_SCANCODE_COUNT];
    bool savedKbDoubleToDo[SDL_SCANCODE_COUNT];
    bool savedKbDoubleActive[SDL_SCANCODE_COUNT];
    float savedMouseButtons[N_MOUSE_BUTTONS];
    bool savedMouseToDo[N_MOUSE_BUTTONS];
    bool savedMouseDoubleToDo[N_MOUSE_BUTTONS];
    bool savedMouseDoubleActive[N_MOUSE_BUTTONS];
    int savedFocus = 0;
};

void ResetState()
{
    for (int i = 0; i < UAN; i++)
    {
        GInput.userKeys[i].Resize(0);
        GInput.userKeysModifiers[i].Resize(0);
        GInput.actionDone[i] = false;
    }
    for (int i = 0; i < SDL_SCANCODE_COUNT; i++)
    {
        GInput.keyboard.keys[i] = 0;
        GInput.keyboard.keysToDo[i] = false;
        GInput.keyboard.keysDoubleTapToDo[i] = false;
        GInput.keyboard.keysDoubleTapActive[i] = false;
    }
    for (int i = 0; i < N_MOUSE_BUTTONS; i++)
    {
        GInput.mouse.buttons[i] = 0.0f;
        GInput.mouse.buttonsToDo[i] = false;
        GInput.mouse.buttonsDoubleToDo[i] = false;
        GInput.mouse.buttonsDoubleActive[i] = false;
    }
    GInput.gameFocusLost = 0;

    auto& sub = InputSubsystem::Instance();
    sub.SetContext(InputContext::Menu);
    sub.GetProfile(InputContext::Menu).ClearAll();
}

// Mirrors what BindingsPage::ApplyCapture does when the user saves a
// captured (code, modifier) pair — single source of truth for the
// "user rebound Fire to Ctrl+C" scenario this test simulates.
void BindCombo(UserAction action, int code, int modifier)
{
    GInput.userKeys[action].Resize(0);
    GInput.userKeys[action].Add(code);
    GInput.userKeysModifiers[action].Resize(0);
    GInput.userKeysModifiers[action].Add(modifier);

    auto& profile = InputSubsystem::Instance().GetProfile(InputContext::Menu);
    profile.ClearBindings(action);
    InputCode modCode = modifier >= 0 ? InputCode::FromLegacy(modifier) : InputCode{};
    profile.Bind(action, InputBinding(InputCode::FromLegacy(code), modCode));
}
} // namespace

TEST_CASE("Regression: Ctrl+C bound to Fire fires when both held", "[Input][ComboRegression]")
{
    FullStateSnapshot snap;
    auto& sub = InputSubsystem::Instance();
    ResetState();

    // Mirror what the user does in the new Settings UI: rebind UAFire
    // to Ctrl+C (LCtrl modifier + C key).
    BindCombo(UAFire, (int)SDL_SCANCODE_C, (int)SDL_SCANCODE_LCTRL);

    // Press C alone — modifier requirement fails, no fire.
    GInput.keyboard.keys[SDL_SCANCODE_C] = 1.0f;
    GInput.keyboard.keysToDo[SDL_SCANCODE_C] = true;
    GInput.actionDone[UAFire] = false;
    CHECK(sub.GetActionToDo(UAFire, false, false) == false);
    CHECK(sub.GetAction(UAFire, false) == 0.0f);

    // Add LCtrl held — both conditions met, fires.
    GInput.keyboard.keys[SDL_SCANCODE_LCTRL] = 1.0f;
    GInput.keyboard.keysToDo[SDL_SCANCODE_C] = true;
    GInput.actionDone[UAFire] = false;
    CHECK(sub.GetAction(UAFire, false) > 0.0f);
    CHECK(sub.GetActionToDo(UAFire, false, false) == true);

    // GetActionToDo with reset=true marks actionDone — second call
    // returns false (consumed-once edge semantics that the gameplay
    // pipeline relies on so a single key press doesn't fire twice).
    GInput.actionDone[UAFire] = false;
    CHECK(sub.GetActionToDo(UAFire, true, false) == true);
    CHECK(sub.GetActionToDo(UAFire, true, false) == false);
}

TEST_CASE("Regression: Ctrl+C doesn't fire when LCtrl-bound action is unbound", "[Input][ComboRegression]")
{
    // Sanity: pressing Ctrl+C with Fire unbound shouldn't fire anything.
    FullStateSnapshot snap;
    auto& sub = InputSubsystem::Instance();
    ResetState();

    GInput.keyboard.keys[SDL_SCANCODE_LCTRL] = 1.0f;
    GInput.keyboard.keys[SDL_SCANCODE_C] = 1.0f;
    GInput.keyboard.keysToDo[SDL_SCANCODE_C] = true;

    CHECK(sub.GetAction(UAFire, false) == 0.0f);
    CHECK(sub.GetActionToDo(UAFire, false, false) == false);
}

TEST_CASE("Regression: legacy LCtrl Fire binding still fires when Ctrl alone is held", "[Input][ComboRegression]")
{
    // Original 1.99 default: UAFire bound to bare LCtrl (no modifier).
    // Ensures the modifier-aware code path doesn't accidentally break
    // legacy bare-key bindings — important because BindCombo above is
    // the rebind path; without it the engine still ships the bare
    // default and existing players see no behavior change.
    FullStateSnapshot snap;
    auto& sub = InputSubsystem::Instance();
    ResetState();

    BindCombo(UAFire, (int)SDL_SCANCODE_LCTRL, -1); // bare key, no modifier

    GInput.keyboard.keys[SDL_SCANCODE_LCTRL] = 1.0f;
    GInput.keyboard.keysToDo[SDL_SCANCODE_LCTRL] = true;

    CHECK(sub.GetAction(UAFire, false) > 0.0f);
    CHECK(sub.GetActionToDo(UAFire, false, false) == true);
}

TEST_CASE("Regression: Ctrl+C and bare C coexist as different actions", "[Input][ComboRegression]")
{
    // The whole point of the modifier check: two actions can share the
    // same main key with / without a modifier.  Press C → Reload fires;
    // press Ctrl+C → Fire fires; both can be valid bindings on the
    // same keyboard at the same time.
    FullStateSnapshot snap;
    auto& sub = InputSubsystem::Instance();
    ResetState();

    BindCombo(UAFire, (int)SDL_SCANCODE_C, (int)SDL_SCANCODE_LCTRL);
    BindCombo(UAReloadMagazine, (int)SDL_SCANCODE_C, -1);

    // Bare C — only Reload fires.
    GInput.keyboard.keys[SDL_SCANCODE_C] = 1.0f;
    CHECK(sub.GetAction(UAReloadMagazine, false) > 0.0f);
    CHECK(sub.GetAction(UAFire, false) == 0.0f);

    // Add Ctrl — both fire (Reload doesn't care about Ctrl since its
    // binding is bare; Fire fires because Ctrl+C now matches).
    GInput.keyboard.keys[SDL_SCANCODE_LCTRL] = 1.0f;
    CHECK(sub.GetAction(UAReloadMagazine, false) > 0.0f);
    CHECK(sub.GetAction(UAFire, false) > 0.0f);
}

TEST_CASE("Regression: double-tap keyboard binding fires only from double-tap state", "[Input][ComboRegression]")
{
    FullStateSnapshot snap;
    auto& sub = InputSubsystem::Instance();
    ResetState();

    BindCombo(UAFire, InputBindingDoubleTapCode((int)SDL_SCANCODE_G), -1);

    GInput.keyboard.keys[SDL_SCANCODE_G] = 1.0f;
    GInput.keyboard.keysToDo[SDL_SCANCODE_G] = true;
    CHECK(sub.GetAction(UAFire, false) == 0.0f);
    CHECK(sub.GetActionToDo(UAFire, false, false) == false);

    GInput.keyboard.keysDoubleTapActive[SDL_SCANCODE_G] = true;
    GInput.keyboard.keysDoubleTapToDo[SDL_SCANCODE_G] = true;
    GInput.actionDone[UAFire] = false;
    CHECK(sub.GetAction(UAFire, false) > 0.0f);
    CHECK(sub.GetActionToDo(UAFire, false, false) == true);
}

TEST_CASE("Regression: double-click mouse binding fires only from double-click state", "[Input][ComboRegression]")
{
    FullStateSnapshot snap;
    auto& sub = InputSubsystem::Instance();
    ResetState();

    BindCombo(UAFire, InputBindingDoubleTapCode(INPUT_DEVICE_MOUSE + 1), -1);

    GInput.mouse.buttons[1] = 1.0f;
    GInput.mouse.buttonsToDo[1] = true;
    CHECK(sub.GetAction(UAFire, false) == 0.0f);
    CHECK(sub.GetActionToDo(UAFire, false, false) == false);

    GInput.mouse.buttonsDoubleActive[1] = true;
    GInput.mouse.buttonsDoubleToDo[1] = true;
    GInput.actionDone[UAFire] = false;
    CHECK(sub.GetAction(UAFire, false) > 0.0f);
    CHECK(sub.GetActionToDo(UAFire, false, false) == true);
}
