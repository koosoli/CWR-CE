// Merged from briefing_skip_altenter_stable and briefing_gear_page_loads.
// Both drive the same demo route: SINGLE MISSION → Play → Esc-skip → briefing notebook.
// Shared boot saves one full game-start cycle (~60s).
//
// Part 1 (briefing_skip_altenter_stable): the map pixel at (0.75, 0.25) must survive
// an Alt+Enter fullscreen roundtrip.  Broken-state delta: pixel read ~91,78,81 before
// AltEnter then jumped to ~195,224,249 after (channel delta ~100-170); tol=80 sits
// below that while tolerating 8x-MSAA re-rasterize drift (up to ~52 per channel).
//
// Part 2 (briefing_gear_page_loads): switching to the Gear tab (IDC_MAP_GEAR=59)
// must render weapon icons, not blank paper.  Broken page background (~185,190,155)
// vs. icon silhouette (~16,14,18): channel delta ~170; tol=40 separates them.

triSetLanguage "English"

triAssertEq [(triDisplay), 0]
triClickText "SINGLE MISSION"
triAssertEq [(triDisplay), 2]
triClickText "Play"
triSendKey 41    // Esc — skip intro cutscene
triAssertIncludes [(triVisibleTexts), "Ambush (Demo)"]
triScreenshot "01_briefing_default"

// Part 1: Alt+Enter stability — latch on map tab, roundtrip, reassert.
private _latch = triPixelLatch [0.75, 0.25]
triSendAltEnter
triSimFrames 30
triSendAltEnter
triSimFrames 120
triScreenshot "02_after_altenter_roundtrip"
triAssertNear [(triSamplePixel [0.75, 0.25]), _latch, 80]

// Part 2: Gear tab — switch, settle, assert icon pixel is rendered.
triClick 59
triWaitFrames 30
triScreenshot "03_briefing_gear"
triAssertGt [(triGetBackBufferNonBlackCount), 0]
triAssertNear [(triSamplePixel [0.10, 0.33]), "16,14,18", 40]

triEndTest
