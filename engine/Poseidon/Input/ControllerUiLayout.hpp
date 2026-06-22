#pragma once

#include <Poseidon/Input/ControllerUiInput.hpp>
#include <Poseidon/Foundation/Time/Time.hpp>

#include <SDL3/SDL_keycode.h>

namespace Poseidon
{

enum class ControllerUiDispatchKind
{
    None,
    KeyTap,
    PointerPrimaryDown,
    PointerPrimaryUp,
    PointerPrimaryClick,
    PreviousTab,
    NextTab,
    Preview,
    Pause,
};

struct ControllerUiDispatch
{
    ControllerUiDispatchKind kind = ControllerUiDispatchKind::None;
    SDL_Keycode key = SDLK_UNKNOWN;
};

class ControllerUiRepeater
{
  public:
    bool ShouldEmitDirection(ControllerUiAction action, bool pressed, Foundation::UITime now);
    void ResetDirection(ControllerUiAction action);
    void ResetDirections();

  private:
    struct RepeatState
    {
        bool held = false;
        bool repeating = false;
        Foundation::UITime next = UITIME_MIN;
    };

    RepeatState& StateFor(ControllerUiAction action);
    static bool IsDirection(ControllerUiAction action);

    RepeatState up_;
    RepeatState down_;
    RepeatState left_;
    RepeatState right_;
};

class ControllerMenuUiLayout
{
  public:
    ControllerUiDispatch Map(ControllerUiAction action) const;
};

class ControllerEditorUiLayout : public ControllerUiRepeater
{
  public:
    ControllerUiDispatch Map(ControllerUiAction action) const;
};

class ControllerInGameUiLayout
{
  public:
    ControllerUiDispatch Map(ControllerUiAction action) const;
};

} // namespace Poseidon
