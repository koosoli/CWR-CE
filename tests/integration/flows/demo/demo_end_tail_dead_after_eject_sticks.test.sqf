// Synthetic stuck path:
// Konec is latched and the player exits, but dies before Vyskakali can use the alive gate.
//
// The 90-frame settle is the absence-window for the negative checks
// (Ukonci must NOT be true, CONTINUE must NOT transition).  Shorter
// than the original 180 — mission scripts run within ~1s, the
// extra 1.5s budget catches a delayed transition without paying 3s.

triSceneReady
triAssertEq [(triGetEndMode), "CONTINUE"]
triSimFrames 90
if (Konec == 1) then { "OK" } else { format ["FAIL:Konec=%1", Konec] }
if (alive aP) then { "FAIL:alive_true" } else { "OK" }
if (Ukonci) then { "FAIL:Ukonci_true" } else { "OK" }
triAssertEq [(triGetEndMode), "CONTINUE"]
triEndTest
