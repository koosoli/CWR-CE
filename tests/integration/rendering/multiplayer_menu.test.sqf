// I-20 GL error gate + multiplayer menu — merged from gl_error_gate.test.sqf.
//
// Walks main menu → multiplayer → back, asserting no KHR_debug HIGH-severity GL
// errors at each step. Screenshots the multiplayer menu. Any tri test can append
// triAssertNoGLErrors after a step to gate that specific navigation.
//
// Broken-state: a regression that triggers GL_INVALID_OPERATION during a menu
// transition logs via the KHR_debug callback but does not fail any test without
// this gate. With it: the test fails at the exact step that introduced the error.

triResetGLErrorBaseline
triAssertEq [(triDisplay), 0]
triAssertEq [(triGetGLErrorCount), 0]
triClick 105
triAssertEq [(triDisplay), 8]
triAssertEq [(triGetGLErrorCount), 0]
triScreenshot "multiplayer_menu"
triClick 2
triAssertEq [(triDisplay), 0]
triAssertEq [(triGetGLErrorCount), 0]
triClick 106
