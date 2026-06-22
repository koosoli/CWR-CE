// Remote (workshop) mods: the MODS screen fetches the PAPA BEAR catalog
// (papa-bear.cz / --master-server) on a background thread and merges the results as
// Workshop rows alongside the local scan. A not-yet-downloaded Workshop row is
// Missing, so it is shown and tickable but does not mount until downloaded (Apply
// opens the download dialog for it first — see mods_download.test.sqf).
//
// The live fetch is skipped under autotest (no network in tests); triSeedWorkshopMods
// injects catalog entries through the SAME MergeWorkshopMods path the worker uses, so
// the merge/show/source logic is exercised deterministically. The local fixture mod
// (--mods-dir) is present too, so we can prove Workshop and Local rows coexist.

triSetLanguage "English"
triSimUntil { triGameMode == 2 }
triAssertEq [(triDisplay), 0]

triClick 119                 // IDC_MAIN_MODS
triAssertEq [(triDisplay), 72]          // IDD_MODS

triWaitFrames 10
_local = triModsVisibleCount
if (_local != 1) exitWith { format ["FAIL:local=%1 (want 1 fixture mod, workshop fetch off in autotest)", _local] }

// Merge 3 remote catalog entries -> Workshop rows next to the Local one
triSeedWorkshopMods 3
triWaitFrames 5
_all = triModsVisibleCount
if (_all != 4) exitWith { format ["FAIL:merged=%1 (want 4 = 1 local + 3 workshop)", _all] }
triScreenshot "01_local_plus_workshop"

// Source filter proves the per-row source: 3 Workshop, 1 Local
triClick 122                 // Source: All -> Workshop
triAssertIncludes [(triVisibleTexts), "Source: Workshop"]
_ws = triModsVisibleCount
if (_ws != 3) exitWith { format ["FAIL:workshop=%1 (want 3)", _ws] }
triScreenshot "02_workshop_only"

triClick 122                 // -> Local
triAssertIncludes [(triVisibleTexts), "Source: Local"]
_lo = triModsVisibleCount
if (_lo != 1) exitWith { format ["FAIL:local-filter=%1 (want 1)", _lo] }

triClick 122                 // -> back to All (so the workshop rows are tickable)
triAssertIncludes [(triVisibleTexts), "Source: All"]

// Not-downloaded: ticking a Workshop mod selects it but it is NOT in the mount set
// (it is still Missing on disk); only ticked mods that are downloaded mount. Rows
// are name-sorted: 0=Fixture Mod, 1=Workshop Mod 1.
triModsRowClick [1, 0.03]     // tick Workshop Mod 1
triAssertEq [(triGetModsActiveSet), "wsmod1"]   // it IS selected (ticked)
triAssertEq [(triGetModsMountSet), ""]           // but Apply would mount nothing (workshop ignored)
triModsRowClick [0, 0.03]     // also tick the local Fixture Mod
triAssertEq [(triGetModsMountSet), "fixturemod"]   // only the local mod mounts
triEndTest
