// Viewer auto-frame smoke: boot viewer mode with the synthetic P3D fixture and
// verify the render path produces a non-blank frame.

triSimFrames 6
triAssertGe [(triGetPixelMaxChannel [0.5, 0.5]), 20]
triScreenshot "viewer_auto_frame"
triEndTest
