// Force English locale at start so subsequent triClickText / triAssertText
// against UI labels are stable regardless of the user's persistent
// ColdWarAssault.cfg language setting.
triSetLanguage "English"

triAssertEq [(triDisplay), 0]
triAssertIncludes [(triVisibleTexts), "SINGLE MISSION"]
triScreenshot "01_main_menu"

// The demo config leaves CfgCredits.cutscene empty, so the shared options shell
// hides the Credits row instead of launching a missing cutscene.
triClickText "OPTIONS"
triAssertEq [(triDisplay), 9099]
triAssertIncludes [(triVisibleTexts), "Audio"]
triAssertIncludes [(triVisibleTexts), "Display"]
triAssertIncludes [(triVisibleTexts), "Graphics"]
triAssertIncludes [(triVisibleTexts), "Game"]
triAssertIncludes [(triVisibleTexts), "Back"]
triAssertExcludes [(triVisibleTexts), "Credits"]
triScreenshot "02_options_open_no_credits"

triEndTest
