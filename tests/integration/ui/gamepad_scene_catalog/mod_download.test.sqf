// Temporary controller scene catalog capture for the mod download modal.
// Use the real Mods Apply gate instead of opening the synthetic modal directly;
// that matches the current user path and keeps the route short.

triSetLanguage "Italian"
triSimUntil { triGameMode == 2 }
triAssertEq [(triDisplay), 0]

triClick 119
triAssertEq [(triDisplay), 72]
triWaitFrames 10

triSeedWorkshopMods 2
triWaitFrames 5
triModsRowClick [1, 0.03]
triWaitFrames 5
triClick 115
triAssertEq [(triDisplay), 74]
triScreenshot "13_mod_download_modal"

triClick 2
triAssertEq [(triDisplay), 72]
triEndTest
