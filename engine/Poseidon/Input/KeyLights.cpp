#include <Poseidon/Input/KeyLights.hpp>

#if _ENABLE_CHEATS

#include <SDL3/SDL.h>

namespace Poseidon
{

bool KeyLights::GetScrollLock()
{
    return (SDL_GetModState() & SDL_KMOD_SCROLL) != 0;
}

bool KeyLights::GetCapsLock()
{
    return (SDL_GetModState() & SDL_KMOD_CAPS) != 0;
}

void KeyLights::SetScrollLock(bool) {}
void KeyLights::SetCapsLock(bool) {}

void KeyLights::GetGlobalState()
{
    if (_acquired)
        return;
    _globalScrollLock = GetScrollLock();
    _globalCapsLock = GetCapsLock();
    _acquired = true;
}

void KeyLights::RestoreGlobalState()
{
    if (!_acquired)
        return;
    _acquired = false;
}

KeyLights::KeyLights()
{
    _acquired = false;
    GetGlobalState();
}

KeyLights::~KeyLights()
{
    RestoreGlobalState();
}

} // namespace Poseidon
#endif
