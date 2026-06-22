// Temporary controller scene catalog capture for the main-menu Start shortcut.

triSetLanguage "English"
triAssertEq [(triDisplay), 0]
triAssertEq [(triGetControllerScene), "Menu"]
triAssertIncludes [(triGetControllerPrompts), "Start Options"]
triGpadButton 9
triAssertEq [(triDisplay), 9099]
triEndTest
