// In-process re-mount from the main menu, end to end through the live engine + UI.
//
// The main-menu background is a real 3D scene (GModeIntro, ==2) with meshes resident
// in the global ShapeBank. triRemount requests a full content reload; it is deferred to
// a safe between-frames point (AppIdle), so we pump frames and wait for the menu to come
// back with its scene loaded. We re-mount twice and assert the engine survives each cycle
// and the shape cache stays bounded (no runaway growth reload-over-reload).
//
// NOTE: the exact "clean baseline" count is checked deterministically by the CLI
// `--reload-clean-selftest` (captured at a fixed point); here the intro cutscene animates,
// so the live count fluctuates and we assert a bound rather than equality.

triSetLanguage "English"
triSimUntil { triGameMode == 2 }
triSimUntil { triLoadedShapeCount > 0 }
_base = triLoadedShapeCount

// First re-mount (deferred -> serviced by AppIdle; pump frames to run + rebuild).
_r1 = triRemount
if (_r1 != "OK") exitWith { format ["FAIL:remount1=%1 mode=%2", _r1, triGameMode] }
triSimFrames 60
triSimUntil { triGameMode == 2 && triLoadedShapeCount > 0 }
_after1 = triLoadedShapeCount

// Second re-mount — the menu must survive repeated in-place reloads.
_r2 = triRemount
if (_r2 != "OK") exitWith { format ["FAIL:remount2=%1 mode=%2", _r2, triGameMode] }
triSimFrames 60
triSimUntil { triGameMode == 2 && triLoadedShapeCount > 0 }
_after2 = triLoadedShapeCount

// Engine survived both reloads (live menu scene), and content did not run away
// (a leak would compound: bound at 4x the first observed count).
_bound = _base * 4 + 64
if (triGameMode == 2 && _after1 > 0 && _after2 > 0 && _after1 <= _bound && _after2 <= _bound) then { "OK" } else { format ["FAIL:base=%1 after1=%2 after2=%3 bound=%4 mode=%5", _base, _after1, _after2, _bound, triGameMode] }
triEndTest
