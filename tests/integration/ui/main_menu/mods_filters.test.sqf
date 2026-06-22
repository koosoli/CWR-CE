// MODS catalog filters: source filter cycling and name filter dialog.
// triSeedMods alternates Workshop/Local, so 6 seeded = 3 Workshop + 3 Local.

triSetLanguage "English"
triSimUntil { triGameMode == 2 }
triAssertEq [(triDisplay), 0]

triClick 119
triAssertEq [(triDisplay), 72]

triSeedMods 6

// Source filter: All (6) → Workshop (3) → Local (3) → All (6).
triAssertIncludes [(triVisibleTexts), "Source: All"]
_all = triModsVisibleCount
if (_all != 6) exitWith { format ["FAIL:all count=%1 (want 6)", _all] }
triScreenshot "01_source_all"

triClick 122
triAssertIncludes [(triVisibleTexts), "Source: Workshop"]
_ws = triModsVisibleCount
if (_ws != 3) exitWith { format ["FAIL:workshop count=%1 (want 3)", _ws] }
triScreenshot "02_source_workshop"

triClick 122
triAssertIncludes [(triVisibleTexts), "Source: Local"]
_lo = triModsVisibleCount
if (_lo != 3) exitWith { format ["FAIL:local count=%1 (want 3)", _lo] }
triScreenshot "03_source_local"

triClick 122
triAssertIncludes [(triVisibleTexts), "Source: All"]
_all2 = triModsVisibleCount
if (_all2 != 6) exitWith { format ["FAIL:source-all-again count=%1 (want 6)", _all2] }

// Name filter: "Test Mod 1" matches one row; clear restores all.
triModsSetFilter "Test Mod 1"
triAssertIncludes [(triVisibleTexts), "Filter: Test Mod 1"]
_one = triModsVisibleCount
if (_one != 1) exitWith { format ["FAIL:filtered count=%1 (want 1)", _one] }
triScreenshot "04_name_filtered"

triModsSetFilter ""
triAssertIncludes [(triVisibleTexts), "Filter"]
_all3 = triModsVisibleCount
if (_all3 != 6) exitWith { format ["FAIL:cleared count=%1 (want 6)", _all3] }

// Filter dialog chrome: clicking IDC_MODS_FILTER opens IDD_MODS_FILTER; Cancel
// returns to the catalog.
triClick 123
triAssertEq [(triDisplay), 73]
triScreenshot "05_filter_dialog"
triClick 2
triAssertEq [(triDisplay), 72]

triEndTest
