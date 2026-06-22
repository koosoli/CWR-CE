#pragma once

#include <cstdint>

namespace Poseidon
{

// Input context — determines which action bindings are active.
enum class InputContext : uint8_t
{
    Menu,           // main menu, options, briefing
    Infantry,       // on foot (default gameplay)
    CarDriver,      // driving a car/truck
    TankDriver,     // driving a tank (differential steering)
    TankGunner,     // turret control
    HeliPilot,      // helicopter flight
    PlanePilot,     // airplane flight
    ShipDriver,     // boat/ship
    Gunner,         // static/mounted weapon
    Spectator,      // camera, post-death
    Map,            // map screen
    Chat,           // chat input
    Editor,         // mission editor
    Count,
};
} // namespace Poseidon

