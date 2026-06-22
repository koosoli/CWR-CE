// fizzy #113: pressing Revert on the ConfirmRevertPage (a user-initiated revert,
// not the timeout path) must put the engine back to the mode it had before Apply.
// Apply briefly switches the engine to Exclusive Fullscreen; Revert must restore
// the original Windowed mode on both the engine and the page's row state.
//
// Window Mode stepper: text=511, next=519. Apply=563. ConfirmRevertPage: Keep=9201,
// Revert=9202. Display row in Options=1102.

#include "../../../helpers/options_preamble.sqf"
#include "../../../helpers/display_preamble.sqf"

// Boots windowed (tri injects --window).
if ((triGetWindowMode) != "windowed") exitWith { format ["FAIL:boot_not_windowed mode=%1", triGetWindowMode] };

// Pick Exclusive Fullscreen, Apply (engine switches live), then Revert.
for "_i" from 0 to 3 do { if ((triControlText 511) == "Exclusive Fullscreen") exitWith {}; triClick 519; triSimFrames 2 };
if ((triControlText 511) != "Exclusive Fullscreen") exitWith { format ["FAIL:cycle actual=%1", triControlText 511] };

triClick 563
triSimFrames 10
triAssertIncludes [(triVisibleTexts), "Keep these display settings?"]

// The change is live (fullscreen) while the modal is up; Revert restores the
// pre-Apply windowed mode rather than keeping it.
triClick 9202
triSimFrames 10

if ((triGetWindowMode) != "windowed") exitWith { format ["FAIL:revert_did_not_restore mode=%1", triGetWindowMode] };
triAssertEq [(triGetWindowMode), "windowed"]
// The page's Window Mode row also snaps back (pending reverted to applied).
if ((triControlText 511) != "Window") exitWith { format ["FAIL:row_not_reverted actual=%1", triControlText 511] };
"OK"

triEndTest
