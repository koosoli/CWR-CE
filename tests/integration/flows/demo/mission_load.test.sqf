// Force English locale at start so subsequent triClickText / triAssertText
// against UI labels are stable regardless of the user's persistent
// ColdWarAssault.cfg language setting.
triSetLanguage "English"

// Text-format mission.sqm briefingName values with "$STR_..." tokens resolve via
// stringtable.utf8.csv just like the binary "@STRN_…" path; raw $STR_ tokens
// must never appear in the single-mission notebook.
triAssertEq [(triDisplay), 0]
triClickText "SINGLE MISSION"
triAssertEq [(triDisplay), 2]
// The 3D notebook cover opens asynchronously.  Its mission title/body are
// rendered by the 3D book, not exposed as normal control text.  Keep the
// screenshots here; text/localization assertions live in the briefing tests.
triWaitFrames 200
triScreenshot "01_demo_book_en"

triSetLanguage "Czech"
triWaitFrames 30
triScreenshot "02_demo_book_cz"

triSetLanguage "English"
triWaitFrames 30

// Demo.Demo mission launches and resolves its demo.wrp world from Remaster data.
// triAssertNear polls until VD stabilises without a fixed-frame wait.
triClickText "Play"
// Demo.Demo/mission.sqm ships viewDistance=900 (the OFP default); init.sqs
// does not override it, so the mission settles there once loaded.
triAssertNear [(triGetViewDistance), 900, 1]
triScreenshot "03_in_mission"

triEndTest
