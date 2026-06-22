// Live aspect world-viewport crops render the 3D world only inside the selected
// flat_quad and paint the cropped periphery black.
//
// Method: in the main-menu intro scene, sample a left-edge pixel that is the
// live 3D scene at full viewport.  Apply a centered half-width manual crop
// ([0.25,0,0.75,1]) and resample: the pixel now lands in the left black bar,
// so it must (a) have changed from the scene colour and (b) read near-black.
//
// The left-edge pixel changes from scene colour to near-black after applying
// the crop.

triSetLanguage "English"
triSimFrames 60
triScreenshot "00_full"

// Latch a left-edge pixel while the world is full (it shows the 3D scene).
triPixelLatch [0.08, 0.5]

// Centered half-width crop of the 3D world.
triSetManualViewport [0.25, 0.0, 0.75, 1.0]
triSimFrames 20
triScreenshot "01_cropped"

// The left-edge pixel is now inside the black bar: it changed and is dark.
triAssertPixelChanged [0.08, 0.5, 24]
triAssertLt [(triGetPixelMaxChannel [0.08, 0.5]), 16]

// Restore full viewport so we don't leak override state to other tests.
triSetManualViewport []
triSetAspectOverride false
triSimFrames 10

triEndTest
