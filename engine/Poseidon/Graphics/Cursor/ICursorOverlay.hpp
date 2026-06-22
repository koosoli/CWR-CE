#pragma once

namespace Poseidon
{
class Engine;

// ICursorOverlay — strategy interface for whatever the engine paints
// where the user's mouse is.  Different runtime modes need different
// cursors:
//
//   • In-game / menus: the Arrow / scroll / track cursors driven by
//     ControlsContainer::DrawCursor and the seagull-cam aim cursor in
//     InGameUI::DrawHUDNonAI.  These predate the strategy and live in
//     their existing call sites; if a future cleanup wants to fold
//     them in, GameCursorOverlay would be the home.
//
//   • Viewer mode: a thin ring + centre dot, plus an `L` toggle that
//     grabs the OS pointer to the window.  See ViewerCursorOverlay.
//
// World owns one of these as Ref<ICursorOverlay>, picked at init time
// based on AppConfig::IsViewerMode.  Per-frame the world calls
// Draw() — implementations are responsible for their own visuals.
class ICursorOverlay
{
  public:
    virtual ~ICursorOverlay() = default;

    //! Paint the cursor for this frame.  Called from the world's
    //! draw path after the scene + UI passes; the engine is in 2D
    //! draw state and Draw2D quads work as expected.
    virtual void Draw(Engine* engine) = 0;

    //! User pressed the cursor-lock toggle key.  Default: ignore.
    virtual void ToggleLock(Engine* /*engine*/) {}

    //! Report current lock state for the help overlay.  Default: false.
    virtual bool IsLocked() const { return false; }
};

} // namespace Poseidon
