// Regression: Alt+Enter from the mission editor must keep the editor rendering.
//
// A bad runtime fullscreen/windowed transition can leave the editor display
// logically open while the frame only shows the renderer's default blue clear.
// triAssertScreenNotBlack is too weak for that failure because blue is non-black,
// so this test samples stable editor-screen points and requires color variance
// after the transition.

triResetGLErrorBaseline
triSetLanguage "English"
triWaitFrames 5
triAssertDisplay 0
triClick 115
triAssertDisplay 51
triClick 1
triAssertDisplay 26
triWaitFrames 20
triAssertGt [triGetBackBufferNonBlackCount, 0]

triAssertGt [(triGetPixelMaxDiff [0.13, 0.15, 0.87, 0.85]), 40]
triScreenshot "00_editor_before_altenter"

triSendAltEnter
triAssertNe [(triGetWindowMode), "windowed"]
triAssertGe [(triGetWindowWidth), 1024]
triAssertGe [(triGetWindowHeight), 768]
triAssertDisplay 26
triWaitFrames 20
triAssertGt [triGetBackBufferNonBlackCount, 0]
triAssertEq [triGetGLErrorCount, 0]
triScreenshot "01_editor_after_fullscreen"
triAssertGt [(triGetPixelMaxDiff [0.13, 0.15, 0.87, 0.85]), 40]

triSendAltEnter
triAssertEq [(triGetWindowMode), "windowed"]
triAssertDisplay 26
triWaitFrames 20
triAssertGt [triGetBackBufferNonBlackCount, 0]
triAssertEq [triGetGLErrorCount, 0]
triAssertGt [(triGetPixelMaxDiff [0.13, 0.15, 0.87, 0.85]), 40]
triScreenshot "02_editor_after_windowed"

triEndTest
