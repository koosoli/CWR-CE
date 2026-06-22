#include <Poseidon/UI/Options/PressKeyPage.hpp>

#include <Poseidon/Input/InputSubsystem.hpp>
#include <Poseidon/Input/InputDeviceConstants.hpp>
#include <Poseidon/Input/KeyInput.hpp>

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_scancode.h>

namespace Poseidon
{

namespace
{
bool IsModifierOnly(unsigned key)
{
    switch (key)
    {
        case SDLK_LCTRL:
        case SDLK_RCTRL:
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
        case SDLK_LALT:
        case SDLK_RALT:
        case SDLK_LGUI:
        case SDLK_RGUI:
            return true;
        default:
            return false;
    }
}

// SDL keycode → SDL scancode.  v1 storage is the raw SDL scancode in
// the low bits — same encoding the table uses.
SDL_Scancode KeyToScancode(unsigned key)
{
    return SDL_GetScancodeFromKey((SDL_Keycode)key, nullptr);
}

// Snapshot the currently-held modifier scancode.  Returns the canonical
// left variant (LCtrl / LShift / LAlt) regardless of which side was
// pressed — the gameplay resolver checks the exact scancode, so storing
// LCtrl means "Ctrl on the left specifically", which is fine for v1
// (matches the modifier the user pressed at capture time).
int CapturedModifierScancode()
{
    auto& sub = InputSubsystem::Instance();
    if (sub.IsKeyDown(SDL_SCANCODE_LCTRL))
        return (int)SDL_SCANCODE_LCTRL;
    if (sub.IsKeyDown(SDL_SCANCODE_RCTRL))
        return (int)SDL_SCANCODE_RCTRL;
    if (sub.IsKeyDown(SDL_SCANCODE_LSHIFT))
        return (int)SDL_SCANCODE_LSHIFT;
    if (sub.IsKeyDown(SDL_SCANCODE_RSHIFT))
        return (int)SDL_SCANCODE_RSHIFT;
    if (sub.IsKeyDown(SDL_SCANCODE_LALT))
        return (int)SDL_SCANCODE_LALT;
    if (sub.IsKeyDown(SDL_SCANCODE_RALT))
        return (int)SDL_SCANCODE_RALT;
    return -1;
}

int CapturedMouseButtonInput()
{
    auto& sub = InputSubsystem::Instance();
    for (int i = 0; i < N_MOUSE_BUTTONS; ++i)
    {
        if (sub.GetMouseButtonToDo(i))
            return INPUT_DEVICE_MOUSE + i;
    }
    return -1;
}
} // namespace

CapturePage::Result PressKeyPage::InterpretKey(unsigned nChar, int& outPackedCode, int& outModifier) const
{
    if (IsModifierOnly(nChar))
        return Result::Refused;

    SDL_Scancode sc = KeyToScancode(nChar);
    if (sc == SDL_SCANCODE_UNKNOWN)
        return Result::Refused;

    outPackedCode = (int)sc;
    outModifier = CapturedModifierScancode();
    return Result::Main;
}

bool PressKeyPage::OnKeyDown(OptionsShell& shell, unsigned nChar)
{
    int packed = -1;
    int modifier = -1;
    if (InterpretKey(nChar, packed, modifier) == Result::Main && TryUpgradeToDoubleTap(shell, packed, modifier))
        return true;
    return CapturePage::OnKeyDown(shell, nChar);
}

void PressKeyPage::OnSimulate(OptionsShell& shell)
{
    const int mouse = CapturedMouseButtonInput();
    if (mouse < 0)
        return;

    if (IsListening())
        TryCapture(shell, mouse, -1);
    else
        TryUpgradeToDoubleTap(shell, mouse, -1);
}

} // namespace Poseidon
