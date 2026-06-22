#pragma once

// Mouse user-settings persisted to <user-dir>/mouse.cfg.
//
// Sibling to AudioConfig — same Defaults / Normalize / Load / Save
// pattern.  Holds the four binding-screen scalars: Y-invert, button swap, and
// per-axis sensitivity.
//
// Lifecycle:
//   1. Boot: try Load(path).  Missing/corrupt → LoadDefaults() +
//            Save(path).
//   2. Boot: Normalize() clamps sensitivities to [0.5, 2.0] (the
//            range the original sensitivity sliders shipped with).
//   3. UI:   MousePage applies values live and Saves on Unmount.

#include <string>


namespace Poseidon
{
class MouseConfig
{
public:
    bool  reverseY            = false;
    bool  buttonsReversed     = false;
    float sensitivityX        = 1.0f;
    float sensitivityY        = 1.0f;

    // Reset every field to factory defaults.
    void LoadDefaults();

    // Clamp sensitivities to [0.5, 2.0].  Returns true if any field
    // was changed.  No Environment — nothing to query here.
    bool Normalize();

    // True on successful parse, false if the file doesn't exist or is
    // unparseable.
    bool Load(const std::string& path);

    // Writes via ParamFile.  Returns false on I/O error.
    bool Save(const std::string& path) const;
};

} // namespace Poseidon
