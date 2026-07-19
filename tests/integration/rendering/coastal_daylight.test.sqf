// Deterministic Demo-water daylight capture: mission time, weather, fog, and
// seed are fixed by coastal_daylight.Demo/mission.sqm. The Jeep is a known
// solid caster in the pinned coastal view.

triSceneReady
triAssertEq [triEnableShadows, "OK"]
triAssertEq [triEnableShadowMaps, "OK"]
triAssertEq [triSetView [7035.0,8.0,6475.0,0.57,-0.20,0.80,0.0,1.0,0.0], "OK"]
triSimFrames 60

// Noon must keep the daylight shadow path active. The screenshot is retained
// for visual review; do not couple this fixture to platform-sensitive pixels.
triAssertEq [triShadowSunFactor, "OK:1.00"]
triScreenshot "coastal_daylight"
triEndTest
