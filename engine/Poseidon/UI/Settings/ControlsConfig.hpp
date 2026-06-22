#pragma once

// Keyboard / mouse / joystick bindings persisted to <user-dir>/controls.cfg.
//
// Sibling to AudioConfig — same Defaults / Load / Save pattern.  No
// Normalize method: a packed binding int either parses cleanly or it
// doesn't, and the only "validity" rule (deduping inside one slot) is
// trivial enough to live in the writer.
//
// Field convention: one AutoArray<int> per UserAction, indexed by the
// enum value.  Each int is the same INPUT_DEVICE_* | code packed shape
// the engine has used since 1.99 — kept verbatim so the on-disk format
// is human-readable for users who want to edit by hand.
//
// File shape:
//   keyMoveForward[]={26,82};         // SDL_SCANCODE_W, SDL_SCANCODE_UP
//   keyFire[]={224};                  // SDL_SCANCODE_LCTRL
//   ...
//
// Lifecycle:
//   1. Boot: try Load(path).  Missing/corrupt → LoadDefaults() + Save —
//            eager-write so a hand-editable file always exists.
//   2. UI:   Controls page reads/writes the AutoArrays directly via
//            InputSubsystem; on Unmount/Reset the page calls Save.
//
// This is a fresh game.  UserInfo.cfg is not read; key*[] lines
// living there are ignored.

#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Input/UserAction.hpp>

#include <string>


namespace Poseidon
{
class ControlsConfig
{
public:
    // Per-action binding lists, indexed by UserAction.  Public for the
    // same reason AudioConfig's fields are: tests + UI sync.
    AutoArray<int> bindings[UAN];

    // Parallel modifier list — modifiers[a][i] is the optional modifier
    // scancode that must be held alongside bindings[a][i].  -1 = none.
    // Same length as bindings[a]; LoadDefaults seeds them all to -1
    // since the engine UserActionDesc table has no modifier defaults.
    AutoArray<int> modifiers[UAN];

    // Reset every action's binding list to the engine's built-in defaults
    // (the original 1.99 keyboard / mouse defaults, baked into
    // InputSubsystem::GetUserActionDesc()).
    void LoadDefaults();

    // True on successful parse, false if the file doesn't exist or is
    // unparseable.  Same posture as AudioConfig::Load — leaves the
    // instance untouched on failure so callers can chain
    // Load → LoadDefaults → Save.
    bool Load(const std::string& path);

    // Writes via ParamFile.  Returns false on I/O error.
    bool Save(const std::string& path) const;
};

} // namespace Poseidon
