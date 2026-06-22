// fizzy #113: Windowed mode exposes basic window sizes (800x600 and up) that can
// be selected and applied. Applying a window size must leave the engine WINDOWED
// (not fullscreen) and actually resize the window to the chosen size.
//
// The harness boots windowed (tri injects --window), so the page starts on
// "Window" mode and the Resolution row is now enabled (previously it was inert in
// any non-Fullscreen mode).

#include "../../../helpers/options_preamble.sqf"
#include "../../../helpers/display_preamble.sqf"

// Windowed mode enables the Resolution row (the window-size picker).
if ((triAssert [(triGetControlEnabled 522)]) != "OK") exitWith { "FAIL:resolution_disabled_in_windowed" };

private _startSize = triGetWindowSize;
private _nativeLabel = triControlText 521;

// Rotate the Resolution row to a window size that differs from the current window
// size (don't hardcode which — the list is filtered to sizes that fit the desktop).
private _picked = "";
for "_i" from 1 to 7 do {
    triClick 529;          // Resolution → next
    triSimFrames 1;
    private _label = triControlText 521;
    if (_label != _nativeLabel && _label != _startSize) exitWith { _picked = _label };
};
if (_picked == "") exitWith { format ["FAIL:no_distinct_window_size start=%1 native=%2", _startSize, _nativeLabel] };

triClick 563               // Apply
#include "../../../helpers/display_confirm_keep.sqf"

// The core regression: applying a window size must NOT drop us into fullscreen.
triAssertEq [(triGetWindowMode), "windowed"]

// And the window must actually be the size we picked.
private _live = triGetWindowSize;
if (_live != _picked) exitWith { format ["FAIL:size_not_applied picked=%1 live=%2", _picked, _live] };

// A window must be smaller than the desktop (a real window, not screen-filling).
if ((triAssertWindowedSmallerThanDesktop) != "OK") exitWith {
    format ["FAIL:not_smaller picked=%1 desktop=%2", _picked, triGetDesktopDisplayMode]
};
"OK"

triEndTest
