// Synthetic stuck path:
// Konec is latched, the player stays in UH, and the BUM branch fires,
// but the mission must still not advance to Ukonci / END1.

triSceneReady
triAssertEq [(triGetEndMode), "CONTINUE"]
// 120-frame absence-window: the BUM branch fires later than the
// other stuck cases (it needs the player to stay in UH long enough
// to register), so this test keeps a 2s wait vs the other stuck
// tests' 1.5s.
triSimFrames 120
if (Konec == 1) then { "OK" } else { format ["FAIL:Konec=%1", Konec] }
if (aP in UH) then { "OK" } else { format ["FAIL:inUH=%1", aP in UH] }
if (BumTriggered) then { "OK" } else { "FAIL:BumTriggered_false" }
if (Ukonci) then { "FAIL:Ukonci_true" } else { "OK" }
triAssertEq [(triGetEndMode), "CONTINUE"]
triEndTest
