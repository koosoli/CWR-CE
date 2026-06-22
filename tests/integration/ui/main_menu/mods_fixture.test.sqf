// Self-contained MODS scan -> tick -> Apply -> mounted test against a committed
// fixture mod (tests/fixtures/mods/@fixturemod). It needs no LFS pull and exercises
// the mod.json path: @fixturemod carries a manifest, so the row shows the manifest
// "name" ("Fixture Mod"), not the folder name. The mod also carries one tiny
// synthetic addon so mount/config loading is covered without third-party content.
//
// The .toml sidecar points --mods-dir at ../../tests/fixtures/mods (game CWD under
// tri = the data dir packages/Remaster).

triSetLanguage "English"
triSimUntil { triGameMode == 2 }
triAssertEq [(triDisplay), 0]

triClick 119                 // IDC_MAIN_MODS
triAssertEq [(triDisplay), 72]          // IDD_MODS

triWaitFrames 10
_n = triModsVisibleCount
if (_n != 1) exitWith { format ["FAIL:visible=%1 (want 1 fixture mod)", _n] }
triScreenshot "01_fixture_mod"

// Tick + Apply -> in-process re-mount with the fixture mod
triModsRowClick [0, 0.03]
triAssertEq [(triGetModsActiveSet), "fixturemod"]
triScreenshot "02_ticked"

triClick 115                 // IDC_MODS_APPLY
triSimFrames 60
triSimUntil { triGameMode == 2 && triLoadedShapeCount > 0 }
triAssertEq [(triDisplay), 0]
triAssertIncludes [(triGetActiveMods), "fixturemod"]   // mounted: the fixture mod is in the active mod path
triEndTest
