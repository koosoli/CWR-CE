// fizzy #113: a full window-mode round-trip with Apply + Keep after each step must
// land the engine in the chosen mode every time, and end back where it started.
// Walks Window -> Fullscreen (Borderless) -> Exclusive Fullscreen -> Window and
// asserts the live engine mode (triGetWindowMode) after each Apply + Keep.
//
// UI label -> internal mode: "Window" = windowed, "Fullscreen" = borderless,
// "Exclusive Fullscreen" = fullscreen.  Window Mode stepper: text=511, next=519.
// Apply action=563, ConfirmRevertPage Keep=9201, Display row in Options=1102.

#include "../../../helpers/options_preamble.sqf"
#include "../../../helpers/display_preamble.sqf"

// --- Step 1: Fullscreen label (Borderless) ---------------------------------
for "_i" from 0 to 3 do { if ((triControlText 511) == "Fullscreen") exitWith {}; triClick 519; triSimFrames 2 };
if ((triControlText 511) != "Fullscreen") exitWith { format ["FAIL:cycle_borderless actual=%1", triControlText 511] };
triClick 563
#include "../../../helpers/display_confirm_keep.sqf"
if ((triGetWindowMode) != "borderless") exitWith { format ["FAIL:expected_borderless got=%1", triGetWindowMode] };

// --- Step 2: Exclusive Fullscreen ------------------------------------------
for "_i" from 0 to 3 do { if ((triControlText 511) == "Exclusive Fullscreen") exitWith {}; triClick 519; triSimFrames 2 };
if ((triControlText 511) != "Exclusive Fullscreen") exitWith { format ["FAIL:cycle_fullscreen actual=%1", triControlText 511] };
triClick 563
#include "../../../helpers/display_confirm_keep.sqf"
if ((triGetWindowMode) != "fullscreen") exitWith { format ["FAIL:expected_fullscreen got=%1", triGetWindowMode] };

// --- Step 3: back to Window (Windowed) -------------------------------------
for "_i" from 0 to 3 do { if ((triControlText 511) == "Window") exitWith {}; triClick 519; triSimFrames 2 };
if ((triControlText 511) != "Window") exitWith { format ["FAIL:cycle_windowed actual=%1", triControlText 511] };
triClick 563
#include "../../../helpers/display_confirm_keep.sqf"
if ((triGetWindowMode) != "windowed") exitWith { format ["FAIL:expected_windowed got=%1", triGetWindowMode] };
triAssertEq [(triGetWindowMode), "windowed"]
"OK"

triEndTest
