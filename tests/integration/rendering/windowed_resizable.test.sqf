// Windows started or switched to windowed mode must carry SDL_WINDOW_RESIZABLE.
// Broken-state delta: SetWindowMode(Windowed) called SDL_SetWindowBordered but never
// SDL_SetWindowResizable, so a window that launched fullscreen/borderless and then
// switched to windowed via Options had no resizable flag.
//
// This test starts in borderless (via --borderless extra_arg), switches to windowed
// via Alt+Enter, then asserts triGetResizable is true.

triResetGLErrorBaseline

// Start in borderless/fullscreen; SDL backend naming varies, but this must
// not be windowed until Alt+Enter below.
triAssertNe [(triGetWindowMode), "windowed"]

// Switch to windowed via Alt+Enter.
triSendAltEnter
triAssertEq [(triGetWindowMode), "windowed"]

// The window must carry the resizable flag after the transition.
triAssert [(triGetResizable)]

triAssertEq [(triGetGLErrorCount), 0]
triScreenshot "windowed_resizable"
triEndTest
