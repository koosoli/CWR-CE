// Synthetic stuck path:
// Player exits alive, but Konec never latches, so Vyskakali must not advance to Ukonci.

triSceneReady
triAssertEq [(triGetEndMode), "CONTINUE"]
// 90-frame absence-window for the negative checks (Konec must stay 0,
// Ukonci must stay false, CONTINUE must not transition).
triSimFrames 90
if (Konec == 0) then { "OK" } else { format ["FAIL:Konec=%1", Konec] }
if (alive aP and not (aP in UH)) then { "OK" } else { format ["FAIL:alive=%1,inUH=%2", alive aP, aP in UH] }
if (Ukonci) then { "FAIL:Ukonci_true" } else { "OK" }
triAssertEq [(triGetEndMode), "CONTINUE"]
triEndTest
