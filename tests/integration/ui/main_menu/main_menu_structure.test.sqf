// Main menu structural checks: button captions, active controls, version label,
// and clean Quit Game handling.
//
// (altf4_menu_context is kept separate — it requires no_autotest=true to test
// window-close behaviour without --autotest suppressing the close request.)

triSetLanguage "English"

triAssertEq [(triDisplay), 0]

// Top-level button captions.
triAssertIncludes [(triVisibleTexts), "CAMPAIGN GAME"]
triAssertIncludes [(triVisibleTexts), "OPTIONS"]
triAssertIncludes [(triVisibleTexts), "MULTIPLAYER"]
triAssertIncludes [(triVisibleTexts), "QUIT GAME"]
triAssertIncludes [(triVisibleTexts), "SINGLE MISSION"]
triAssertIncludes [(triVisibleTexts), "MISSION EDITOR"]
triAssertIncludes [(triVisibleTexts), "CONTINUE"]

// Key IDCs are real active-text controls, not silent CStatics.
triAssert [(triGetControlVisible 101)]
triAssert [(triGetControlVisible 106)]
triAssert [(triGetControlVisible 105)]
triAssert [(triGetControlVisible 117)]
triAssert [(triGetControlVisible 115)]
triAssert [(triGetControlVisible 102)]

// Version label IDC is visible.
triAssert [(triGetControlVisible 118)]

// Quit exits cleanly — also covers the exit test.
triClickText "QUIT GAME"
