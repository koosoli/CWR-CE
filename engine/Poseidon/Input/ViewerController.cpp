#include <Poseidon/Input/ViewerController.hpp>
#include <Poseidon/Input/InputSubsystem.hpp>

namespace Poseidon
{

// Adapter: route IViewerInputProvider calls to the live InputSubsystem
// singleton.  Kept private to this TU — tests instantiate their own
// mock provider that doesn't pull SDL.
namespace
{
class InputSubsystemViewerProvider : public IViewerInputProvider
{
  public:
    bool MouseLeft() const override { return InputSubsystem::Instance().IsMouseLeftDown(); }
    bool MouseRight() const override { return InputSubsystem::Instance().IsMouseRightDown(); }
    bool MouseMiddle() const override { return InputSubsystem::Instance().IsMouseMiddleDown(); }
    float MouseDeltaX() const override { return InputSubsystem::Instance().GetMouseDeltaX(); }
    float MouseDeltaY() const override { return InputSubsystem::Instance().GetMouseDeltaY(); }
    float ConsumeWheel() override { return InputSubsystem::Instance().ConsumeCursorScroll(); }
    bool KeyDown(SDL_Scancode k) const override { return InputSubsystem::Instance().IsKeyDown(k); }
    bool KeyPressed(SDL_Scancode k) const override { return InputSubsystem::Instance().IsKeyPressed(k); }
    void ConsumeKeyPress(SDL_Scancode k) override { InputSubsystem::Instance().ConsumeKeyPress(k); }
};
} // namespace

// Charset is intentionally narrow and orthogonal to the gameplay binding
// table.  See ViewerController.hpp for the full mouse + key map.
ViewerControls ViewerController::Poll(IViewerInputProvider& input)
{
    ViewerControls ctrl;

    // Mouse drags — continuous, active only while the corresponding
    // button is held.  No scancode key bindings — gameplay never
    // touches mouse drag while viewer mode is active.
    if (input.MouseLeft())
    {
        ctrl.translateX = input.MouseDeltaX();
        ctrl.translateY = input.MouseDeltaY();
    }
    if (input.MouseRight())
    {
        ctrl.rotateX = input.MouseDeltaX();
        ctrl.rotateY = input.MouseDeltaY();
    }
    // MMB drag → pan (moves the camera in its screen-aligned plane);
    // wheel → zoom (camera along forward).  Convention matches
    // Blender/Maya/most 3D viewers — leaves wheel free for one-shot
    // zoom while MMB drag handles freeform reframing.
    if (input.MouseMiddle())
    {
        ctrl.panX = input.MouseDeltaX();
        ctrl.panY = input.MouseDeltaY();
    }
    ctrl.zoom = input.ConsumeWheel();

    // Analogue scrub — `]` forward, `[` back.  Held-key, not edge.
    ctrl.animScrub = (input.KeyDown(SDL_SCANCODE_RIGHTBRACKET) ? 1.0f : 0.0f) -
                     (input.KeyDown(SDL_SCANCODE_LEFTBRACKET) ? 1.0f : 0.0f);

    // Edge actions — consume the scancode so game-side input handlers
    // for the same frame don't see the key.  Esc in particular is a
    // global panic-quit in the game's binding table, and we don't want
    // to dispatch both the game and the viewer reaction.
    auto edge = [&](SDL_Scancode k)
    {
        if (input.KeyPressed(k))
        {
            input.ConsumeKeyPress(k);
            return true;
        }
        return false;
    };

    ctrl.playPauseAnim = edge(SDL_SCANCODE_SPACE);
    ctrl.resetAnim = edge(SDL_SCANCODE_R);
    ctrl.openAnim = edge(SDL_SCANCODE_O);
    ctrl.reloadTextures = edge(SDL_SCANCODE_F5);
    ctrl.resetViewer = edge(SDL_SCANCODE_F6);
    ctrl.toggleCursorLock = edge(SDL_SCANCODE_L);
    ctrl.exitViewer = edge(SDL_SCANCODE_ESCAPE);

    // Help toggle: F1 is the universal binding (works on every
    // keyboard layout); Shift+/ is the legacy `?` combo for US ANSI.
    // On Czech / DE / FR layouts SDL_SCANCODE_SLASH is a different
    // physical key (`-` on Czech) so the combo silently does
    // nothing — F1 is the working path for non-US users.
    // The latch is per-controller-instance (a member field) so tests
    // don't have to reset hidden static state between cases.
    bool slashDown = input.KeyDown(SDL_SCANCODE_SLASH);
    bool shiftDown = input.KeyDown(SDL_SCANCODE_LSHIFT) || input.KeyDown(SDL_SCANCODE_RSHIFT);
    bool toggleDown = input.KeyDown(SDL_SCANCODE_F1) || (slashDown && shiftDown);
    ctrl.toggleHelp = toggleDown && !_slashLatched;
    _slashLatched = toggleDown;

    return ctrl;
}

ViewerControls ViewerController::Poll()
{
    InputSubsystemViewerProvider live;
    return Poll(live);
}
} // namespace Poseidon
