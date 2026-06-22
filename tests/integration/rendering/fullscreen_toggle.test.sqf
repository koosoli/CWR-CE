// Test: Alt+Enter fullscreen toggle works correctly
// Tests:
//   1. Window→fullscreen: size grows to display resolution
//   2. Fullscreen→window: size shrinks back to original
//   3. No phantom toggles (state stays stable after each transition)
//   4. Rendering stays alive through transitions
//
// All triAssert* poll-until-deadline (Capybara sync), so the per-step
// `triWait 2000` budget the test used to pay after each Alt+Enter is
// covered by the next assertion's own polling.  Stability checks keep
// a short real-time wait (300ms) — they assert ABSENCE of an event
// (no phantom re-toggle), which can't be polled away.

triResetGLErrorBaseline
triAssertEq [(triDisplay), 0]
// Initial windowed state — assertion polls until SDL settles the
// borderless→windowed transition that happens during early init.
triAssertEq [(triGetWindowMode), "windowed"]
triAssertGt [(triGetBackBufferNonBlackCount), 0]

// Toggle to fullscreen/borderless.  Different SDL backends report the
// monitor-sized state as either fullscreen or borderless; the invariant is
// that Alt+Enter leaves windowed mode and grows to display size.
triSendAltEnter
triAssertNe [(triGetWindowMode), "windowed"]
triAssertGe [(triGetWindowWidth), 1024]
triAssertGe [(triGetWindowHeight), 768]
triAssertGt [(triGetBackBufferNonBlackCount), 0]
triAssertEq [(triGetGLErrorCount), 0]
triScreenshot "fullscreen"

// Stability check: real-time pause to catch a phantom re-toggle.
// Can't be polled — we're asserting an event DIDN'T fire.
triWait 300
triAssertNe [(triGetWindowMode), "windowed"]
triAssertGe [(triGetWindowWidth), 1024]
triAssertGe [(triGetWindowHeight), 768]
triAssertGt [(triGetBackBufferNonBlackCount), 0]

// Toggle back to windowed.
triSendAltEnter
triAssertEq [(triGetWindowMode), "windowed"]
triAssertLt [(triGetWindowWidth), 1920]
triAssertLt [(triGetWindowHeight), 1080]
triAssertGt [(triGetBackBufferNonBlackCount), 0]
triAssertEq [(triGetGLErrorCount), 0]
triScreenshot "windowed"

// Same stability beat after the second toggle.
triWait 300
triAssertEq [(triGetWindowMode), "windowed"]
triAssertGt [(triGetBackBufferNonBlackCount), 0]
triAssertEq [(triGetGLErrorCount), 0]

triClick 106
