// fizzy #164: camCreate must create ANY non-AI vehicle at an exact position — not just
// cameras/seagulls. A modder's "logic" camCreate / "radio" camCreate silently returned objNull
// (their flamethrower script only worked after swapping camCreate for createVehicle).
//
// The original engine allowed every type: its `!strcmpi(t,"camera") && !strcmpi(t,"seagull")`
// guard is never both-true, so it never fired. The remaster had "corrected" it to
// `strcmpi && strcmpi` (reject anything but camera/seagull) — the regression. Removed it; an
// unknown type is still rejected naturally (NewNonAIVehicle returns null).
//
// Broken-state delta: with the camera/seagull guard, "Logic" camCreate returns objNull and the
// first assertion FAILs. ("Camera" is the control that worked even while broken.)
//
// fizzy #192: the modern-spelling command aliases must resolve to the same handler as the
// OFP-era originals. An unregistered alias is an unknown operator -> script error ->
// --strict abort (exit 3). getDamage is also value-checked against what setDammage wrote.

// Command spelling aliases (fizzy #192) — wait 1s for game state
triSimUntil { time >= 1 }

player setDammage 0.4
triSimUntil { ((getDamage player) > 0.39) && ((getDamage player) < 0.41) }
_b = behavior player
player setBehavior "AWARE"
_s = player getSelectionDamage "head"
player setSelectionDamage ["head", 0.1]
[group player, 0] setWaypointBehavior "AWARE"

// camCreate any type (fizzy #164) — 40 more frames for engine state
triSimFrames 40
cwrLogicObj = "Logic" camCreate [0,0,0];
if (isNull cwrLogicObj) exitWith { "FAIL:'Logic' camCreate returned objNull (non-camera type wrongly rejected)" };
cwrCamObj = "Camera" camCreate [0,0,0];
if (isNull cwrCamObj) exitWith { "FAIL:'Camera' camCreate returned objNull" };
triEndTest
