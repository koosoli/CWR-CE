#pragma once

#include <Poseidon/World/Scene/Object.hpp>
#include <Poseidon/World/Simulation/Animation/RtAnimation.hpp>

/// Lightweight wrapper around the options-menu notebook (notebook.p3d +
/// notebook.rtm).  Renders the same model with the same open animation
/// and the same camera/zoom shape as the in-game options screen, but
/// without pulling in the UI / ControlsContainer / display stack.
///
/// Lifetime: construct, call `Load()` once (after the graphics engine
/// is up), then `Tick(dt)` + `RenderScene()` each frame.

using namespace Poseidon;
class NotebookScene : public Object
{
public:
    NotebookScene();
    ~NotebookScene() override;

    /// Load the shape, build the skeleton, prepare the open animation.
    /// Returns false (and logs) if any file is missing.
    bool Load();

    /// Advance the open animation phase + the zoom interpolation.
    /// Both saturate at 1.0 after `animSpeed` / `zoomDuration` seconds.
    void Tick(float deltaT);

    /// Push lights + camera and draw the notebook.  Caller is
    /// responsible for `GEngine->InitDraw` / `FinishDraw` framing.
    void RenderScene();

    using Object::Draw;

    /// Apply the open animation to the shape — invoked by Object::Draw
    /// before the mesh is dispatched to the engine.
    void Animate(int level) override;

    bool IsAnimated(int level) const override { return true; }

private:
    Ref<Skeleton> _skeleton;
    Ref<AnimationRT> _animation;
    WeightInfo _weights;

    // Match the OptTplNotebook resource template in
    // resources/menu/optionsTemplates.hpp 1:1 so the on-screen pose,
    // zoom curve, and animation speed are identical to the options
    // menu.
    float _phase = 0.0f;        // 0=closed, 1=open
    float _animSpeed = 1.0f;    // 1 second open→close cycle
    float _zoomT = 0.0f;        // 0=far (positionBack), 1=near (position)
    float _zoomDuration = 1.0f; // matches RscOptionsShell

    bool _loaded = false;
};
