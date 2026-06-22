// Regression for issue #5: after applying a non-native exclusive mode, picking
// "Native" and keeping the change must restore the desktop display mode rather
// than leaving the previous low-resolution exclusive mode active.

#include "../../../helpers/options_preamble.sqf"
#include "../../../helpers/display_preamble.sqf"

private _desktopMode = triGetDesktopDisplayMode;
private _desktopSize = triGetWindowSize;
private _nativeLabel = triControlText 521;

// Other display tests may persist Window / Exclusive state, so rotate until
// Resolution re-enables instead of assuming a specific starting label.
for "_i" from 0 to 3 do {
    if ((triAssert [(triGetControlEnabled 522)]) == "OK") exitWith {};
    triClick 519;
    triSimFrames 2
};
if ((triAssert [(triGetControlEnabled 522)]) != "OK") exitWith { "FAIL:resolution_not_enabled_in_exclusive" };

// Pick the first available non-native mode instead of hardcoding 800x600 —
// some CI runners only advertise a subset of fullscreen modes.
private _pickedMode = "Native";
private _pickedSteps = 0;
for "_i" from 1 to 79 do {
    triClick 529;
    triSimFrames 1;
    private _candidate = triControlText 521;
    if (_candidate != _nativeLabel && _candidate != _desktopSize) exitWith {
        _pickedMode = _candidate;
        _pickedSteps = _i
    }
};
if (_pickedSteps <= 0) exitWith {
    format ["FAIL:no_non_native_mode_available actual=%1 desktop=%2", triControlText 521, _desktopSize]
};

triClick 563
#include "../../../helpers/display_confirm_keep.sqf"
private _lowResCurrentMode = triGetCurrentDisplayMode;
private _lowResRequestedMode = triGetRequestedFullscreenMode;
if (_lowResCurrentMode == _desktopMode && _lowResRequestedMode == _desktopMode) exitWith {
    format ["FAIL:low_res_not_applied current=%1 requested=%2 picked=%3 desktop=%4",
            _lowResCurrentMode, _lowResRequestedMode, _pickedMode, _desktopMode]
};

// Now walk back to Native and apply again.
for "_i" from 1 to _pickedSteps do {
    triClick 528;
    triSimFrames 1
};

triClick 563
#include "../../../helpers/display_confirm_keep.sqf"

private _currentMode = triGetCurrentDisplayMode;
private _requestedMode = triGetRequestedFullscreenMode;
if (_currentMode != _desktopMode && _requestedMode != _desktopMode) exitWith {
    format ["FAIL:native_not_applied current=%1 requested=%2 desktop=%3", _currentMode, _requestedMode, _desktopMode]
};
"OK"

triEndTest
