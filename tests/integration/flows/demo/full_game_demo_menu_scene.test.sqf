// Full PoseidonGame can be pointed at RemasterDemo as a valid data directory.
// RemasterDemo does not ship CfgWorlds::initWorld terrain (Intro), so the menu
// background must fall back to the configured demoWorld instead of staying in a
// menu with no loaded intro scene.

triAssertEq [(triDisplay), 0]
triSimUntil { triGameMode == 2 }
triAssertEq [(triGameMode), 2]
triEndTest
