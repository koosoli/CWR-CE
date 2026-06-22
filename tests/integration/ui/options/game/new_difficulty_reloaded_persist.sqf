// Fresh-process half of the Difficulty persistence regression. The Pester
// wrapper runs this after new_difficulty_persist.test.sqf with the same
// ephemeral user dir, so the values must come from difficulty.cfg.

triSetLanguage "English"
triClickText "OPTIONS"
triClickText "Difficulty"
triAssertDisplay 9099
triAssertIncludes [(triVisibleTexts), "Difficulty"]

if ((triControlText 501) != "Cadet") then { format ["FAIL: expected active mode Cadet after reload, got '%1'", triControlText 501] } else { "OK" }
if ((triControlText 521) != "Disabled") then { format ["FAIL: expected Friendly Tag disabled after reload, got '%1'", triControlText 521] } else { "OK" }
if ((triControlText 531) != "Enabled") then { format ["FAIL: expected Enemy Tag enabled after reload, got '%1'", triControlText 531] } else { "OK" }

triScreenshot "difficulty_tags_reloaded"

triEndTest
