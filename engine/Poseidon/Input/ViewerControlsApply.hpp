#pragma once

#include <Poseidon/Input/ViewerControls.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>

namespace Poseidon
{

// ViewerStateIO — the slice of object/camera/animation state that
// ViewerControls operate on, in/out of ApplyViewerControls.  Pulled
// out as a plain struct so the apply math is a pure free function
// and unit-testable without any engine globals.
struct ViewerStateIO
{
    // Object
    Vector3 objectPos    = VZero;
    Matrix3 objectOrient = M3Identity;
    float   animPhase    = 0.0f;
    float   animSpeed    = 0.0f;

    // Camera
    Vector3 cameraPos       = VZero;
    Vector3 cameraDirection = Vector3(0, 0, 1);  // forward
    Vector3 cameraAside     = Vector3(1, 0, 0);  // right
    Vector3 cameraUp        = Vector3(0, 1, 0);  // up

    // Side-effect flags reported back by the apply step (the caller
    // executes the side-effect, which keeps the apply itself pure).
    bool didReloadTextures = false;
    bool didExitViewer     = false;
};

//! Apply a ViewerControls snapshot to a ViewerStateIO.
//!
//! Pure function — no globals, no I/O.  Takes the previous state,
//! returns the new state.  `defaultAnimSpeed` is the speed used when
//! play/pause toggles from paused → playing (matches
//! AppConfig::GetViewerAnimSpeed in production).
//!
//! Translation uses the camera's right/up axes, so dragging right
//! always moves the object right on screen regardless of its current
//! orientation.  Rotation builds a fresh head/dive matrix from the
//! object's current Direction() and the rotate deltas; pitch is
//! clamped to ±π/2 minus a hair to avoid gimbal flip at the poles.
//! Zoom translates the camera along its forward axis.
ViewerStateIO ApplyViewerControls(const ViewerControls& c, const ViewerStateIO& in, float deltaT,
                                  float defaultAnimSpeed);
} // namespace Poseidon

