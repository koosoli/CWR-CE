#pragma once

// Gamepad button capture modal — sibling to PressKeyPage.  Captures
// stick button presses via the live InputSubsystem polling; modifier
// combos (LB-as-modifier etc.) deferred for v2.
//
// Until the SDL gamepad-event path lands in the harness, this page
// also accepts keyboard scancodes as a debug / dev affordance — the
// captured value is packed via INPUT_DEVICE_STICK + button index when
// the input came from a gamepad button, or the raw scancode otherwise.

#include <Poseidon/UI/Options/CapturePage.hpp>


namespace Poseidon
{
class PressButtonPage : public CapturePage
{
  public:
    PressButtonPage(std::string actionLabel,
                    std::string slotName,
                    SaveCallback onSave,
                    ConflictCallback onConflict)
        : CapturePage(Idcs{9301, 9303, 9302, 9380, 9381},
                      std::move(actionLabel),
                      std::move(slotName),
                      std::move(onSave),
                      std::move(onConflict))
    {
    }

    const char* ResourceClassName() const override { return "RscOptionsPagePressKey"; }

  protected:
    const char* PromptKey()  const override { return "STR_DISP_OPT_CAP_PRESS_BUTTON"; }
    const char* PromptVerb() const override { return "button"; }
    bool OnKeyDown(OptionsShell& shell, unsigned nChar) override;
    Result InterpretKey(unsigned nChar, int& outPackedCode, int& outModifier) const override;
    void OnSimulate(OptionsShell& shell) override;
};

} // namespace Poseidon
