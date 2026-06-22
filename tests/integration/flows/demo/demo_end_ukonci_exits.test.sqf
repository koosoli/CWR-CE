// Synthetic direct-Ukonci regression:
// once the mission has already latched Ukonci=true, StartOutro must run and END1
// must follow. This covers the valid middle state between Vyskakali and uplkonec.

triSceneReady
triAssertEq [(triGetEndMode), "CONTINUE"]
if (Ukonci) then { "OK" } else { "FAIL:Ukonci_false" }
triAssertEq [(triGetEndMode), "END1"]
triEndTest
