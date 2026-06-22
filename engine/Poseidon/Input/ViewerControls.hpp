#pragma once

namespace Poseidon
{

// ViewerControls — normalized control values produced by ViewerController.
// Mirrors the shape of VehicleControls/PilotController but with a charset
// tailored to a model viewer: orbit / pan / zoom + animation transport +
// utility hotkeys.  Kept deliberately disjoint from VehicleControls so the
// viewer never inherits gameplay semantics by accident.
//
// Continuous fields are in [-1, +1] (or [0, +inf] where noted).
// Edge fields are one-frame booleans.
struct ViewerControls
{
    // Continuous — translate (LMB) / rotate (RMB) / pan (MMB) / zoom (wheel)
    float translateX = 0.0f; // LMB drag X — slides object along camera right
    float translateY = 0.0f; // LMB drag Y — slides object along camera up
    float rotateX = 0.0f;    // RMB drag X — yaw (head)
    float rotateY = 0.0f;    // RMB drag Y — pitch (dive)
    float panX = 0.0f;       // MMB drag X — moves camera along its right axis
    float panY = 0.0f;       // MMB drag Y — moves camera along its up axis
    float zoom = 0.0f;       // wheel ticks (positive = zoom in)

    // Continuous — animation transport (kept analogue so future scrubbing
    // bars / gamepad triggers can drive it without re-routing).
    float animScrub = 0.0f;  // [-1, +1] manual phase advance per second

    // One-frame edges — utility actions.  Polled with GetActionToDo /
    // ConsumeKeyPress semantics so they fire exactly once per press.
    bool playPauseAnim    = false;
    bool resetAnim        = false;  // jump to phase 0, pause
    bool openAnim         = false;  // jump to phase 1, pause
    bool reloadTextures   = false;
    bool toggleHelp       = false;
    bool toggleCursorLock = false;  // grab/release mouse to/from window
    bool resetViewer      = false;  // F6 — recenter camera + reset orient/anim
    bool exitViewer       = false;

    void Reset() { *this = ViewerControls{}; }
};
} // namespace Poseidon

