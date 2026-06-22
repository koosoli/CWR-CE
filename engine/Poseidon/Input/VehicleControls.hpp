#pragma once

#include <cstdint>

namespace Poseidon
{

// VehicleControls — normalized vehicle control values produced by PilotController.
// All values in [-1, +1] range unless otherwise noted.
// Not all fields are used by all vehicle types.
struct VehicleControls
{
    // Movement
    float throttle = 0.0f;  // forward/back thrust [-1, +1]
    float steering = 0.0f;  // left/right turn [-1, +1]
    float steeringR = 0.0f; // right-side thrust for differential steering (tanks, ships) [-1, +1]

    // Air controls
    float pitch = 0.0f;      // nose up/down [-1, +1] (elevator for planes, cyclic for helis)
    float roll = 0.0f;       // bank left/right [-1, +1] (aileron for planes, cyclic lateral for helis)
    float yaw = 0.0f;        // rudder / anti-torque [-1, +1]
    float collective = 0.0f; // helicopter collective / vertical thrust [-1, +1]

    // Common actions (one-frame edges)
    bool fire = false;
    bool toggleWeapons = false;
    bool reloadMagazine = false;
    bool lockTarget = false;
    bool headlights = false;
    bool optics = false;

    // View control
    float lookX = 0.0f;      // horizontal look [-1, +1]
    float lookY = 0.0f;      // vertical look [-1, +1]
    bool lookAround = false;  // free look modifier

    // Turbo/slow modifiers
    bool turbo = false;
    bool slow = false;

    void Reset()
    {
        *this = VehicleControls{};
    }
};
} // namespace Poseidon

