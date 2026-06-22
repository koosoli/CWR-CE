// Synthetic extraction-tail happy path:
// Konec is latched, player exits alive, Vyskakali fires, outro runs, END1 follows.
//
// Each `if … else { "FAIL:…" }` line returns a FAIL string until the
// scripted state catches up; the runner auto-retries.  No explicit
// simFrames pad needed between assertions.

triSceneReady
triAssertEq [(triGetEndMode), "CONTINUE"]
if (Konec == 1) then { "OK" } else { format ["FAIL:Konec=%1", Konec] }
if (alive aP and not (aP in UH)) then { "OK" } else { format ["FAIL:alive=%1,inUH=%2", alive aP, aP in UH] }
if (Ukonci) then { "OK" } else { "FAIL:Ukonci_false" }
triAssertEq [(triGetEndMode), "END1"]
triEndTest
