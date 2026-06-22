// End-to-end Difficulty persistence: open Difficulty on the active game mode,
// change the reported Friendly Tag / Enemy Tag values, then leave the page so
// difficulty.cfg reflects the exact settings the running game will use.

triSetLanguage "English"
triClickText "OPTIONS"
triClickText "Difficulty"
triAssertEq [(triDisplay), 9099]
triAssertIncludes [(triVisibleTexts), "Difficulty"]

if ((triControlText 501) != "Cadet") then { format ["FAIL: expected active mode Cadet, got '%1'", triControlText 501] } else { "OK" }
if ((triControlText 521) != "Enabled") then { format ["FAIL: expected Friendly Tag enabled for Cadet, got '%1'", triControlText 521] } else { "OK" }
if ((triControlText 531) != "Disabled") then { format ["FAIL: expected Enemy Tag disabled before toggle, got '%1'", triControlText 531] } else { "OK" }

// First row is selected by default. Move to Friendly Tag and toggle it OFF,
// then move to Enemy Tag and toggle it ON. 2-frame gaps keep scancodes
// from coalescing in one event-pump cycle.
triSendKey 81
triSimFrames 2
triSendKey 81
triSimFrames 2
triSendKey 40
triSimFrames 2
triSendKey 81
triSimFrames 2
triSendKey 40
triSimFrames 2

if ((triControlText 521) != "Disabled") then { format ["FAIL: expected Friendly Tag disabled after toggle, got '%1'", triControlText 521] } else { "OK" }
if ((triControlText 531) != "Enabled") then { format ["FAIL: expected Enemy Tag enabled after toggle, got '%1'", triControlText 531] } else { "OK" }

triScreenshot "difficulty_tags_toggled"

triEndTest
