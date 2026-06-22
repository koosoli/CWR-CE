// Regression: legacy Windows-1250 briefing/overview HTML files (BIS-era format,
// declared via <meta charset="windows-1250">) must render correctly when
// GLanguage=Czech.  CHTMLContainer::Load now slurps the file, transcodes it
// through Poseidon::CodepageForLanguage(GLanguage) to UTF-8, then runs the parser
// on the transcoded buffer — without this the parser leaks raw 0xE1/0xED/0xF8
// bytes into the UTF-8 RString pool and the modern font draws garbage.
//
// A fully Czech word like "Přepadení" only renders correctly if its 'ř' (ř, U+0159)
// reaches the font as the proper UTF-8 sequence 0xC5 0x99 — not the raw W1250
// byte 0xF8.  The triAssertText below checks the listbox row populated from the
// mission's stringtable.utf8.csv (already UTF-8) plus the briefing pane,
// which goes through the new HTML transcoder.

triSetLanguage "English"

// All `triAssert*` and `triClickText` poll-until-deadline, so the
// previous `triSimFrames N` settle calls are redundant: each polling
// command waits for state to converge on its own.

triAssertEq [(triDisplay), 0]
triClickText "SINGLE MISSION"
triAssertEq [(triDisplay), 2]
triClickText "Play"
triSendKey 41    // Esc — skip intro cutscene into briefing

// English path — "Background" header text from briefing.html (W1252).
triAssertIncludes [(triVisibleTexts), "Ambush (Demo)"]
triScreenshot "01_briefing_en"

// Czech path — "Přepadení (Demo)" comes from stringtable.utf8.csv (already UTF-8);
// this exercises the listbox.  The briefing.Czech.html pane goes through the
// legacy-to-UTF-8 transcoder and is screenshotted for visual diff. The action
// row is localized too, so exit through the Czech label rather than the
// English "Cancel".
triSetLanguage "Czech"
triAssertIncludes [(triVisibleTexts), "Přepadení (Demo)"]
triScreenshot "02_briefing_cz"

triClick 2
triEndTest
