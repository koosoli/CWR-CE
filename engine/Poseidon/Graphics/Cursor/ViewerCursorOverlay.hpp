#pragma once

#include <Poseidon/Graphics/Cursor/ICursorOverlay.hpp>

// ViewerCursorOverlay — viewer-mode cursor: a thin ring + centre dot
// painted at the current mouse position via Draw2D solid quads, plus
// an `L` toggle that flips IGraphicsEngine::SetMouseGrab so the user
// can confine the pointer to the window.
//
// Owns its own lock state — no globals, no IsViewerMode() checks
// anywhere outside the construction site (World::Init).

namespace Poseidon
{
class ViewerCursorOverlay : public ICursorOverlay
{
  public:
    void Draw(Engine* engine) override;
    void ToggleLock(Engine* engine) override;
    bool IsLocked() const override { return _locked; }

  private:
    bool _locked = false; // unlocked by default — don't trap pointer on launch
};

} // namespace Poseidon
