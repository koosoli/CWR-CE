// Temporary controller scene catalog captures for briefing, gear, gameplay, and
// in-game map surfaces. The route mirrors existing briefing tests.

triSetLanguage "English"
triAssertEq [(triDisplay), 0]

triClickText "SINGLE MISSION"
triAssertEq [(triDisplay), 2]
triWaitFrames 200
triAssertEq [(triGetControllerScene), "Notebook"]
triAssertEq [(triGetControllerSection), "Pager"]
triAssertIncludes [(triGetControllerPrompts), "LT/RT Page"]
triScreenshot "20_briefing_notebook"

triClickText "Play"
triSendKey 41
triClick 59
triWaitFrames 30
triScreenshot "21_briefing_gear"

triEndTest
