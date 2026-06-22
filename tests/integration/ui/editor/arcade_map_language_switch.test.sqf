// Regression: DisplayArcadeMap (display 26) must refresh ALL localized
// text on a runtime language switch, not just the few state-driven
// labels (EASY/ADVANCED, IDs button, Textures button).
//
// Before the fix, RefreshLanguage() only re-localized the difficulty
// and IDs/Textures buttons.  The top mode bar (Units/Groups/Sensors/
// Waypoints/Synchronize/Markers — a CToolBox whose strings[] came from
// the resource as $STR_DISP_ARCMAP_* tokens) and every static resource-
// text button (Load/Save/Clear/Merge/Preview/Continue/Cancel) stayed
// stuck on the boot-time language.  The Mission/Intro/Outro selector
// (CCombo populated programmatically in OnCreateCtrl with
// LocalizeString(IDS_SECTION_*) ) was also stuck.
//
// The fix:
//   1. CToolBox now caches its ParamEntry and overrides ReloadLocalizedText
//      to re-resolve strings[] from it (same pattern as CStatic/CButton/
//      CActiveText/C3DStatic).
//   2. DisplayArcadeMap::RefreshLanguage delegates to the inherited
//      ControlsContainer::RefreshLocalizedText() so every child control
//      (toolbox + buttons) picks up the new language in one shot.
//   3. The Section combo is re-populated explicitly (no ParamEntry-
//      backed text path; programmatic AddString).
//
// Also asserts the new STR_DISP_ARCUNIT_CLASS_* keys (Insert Unit
// dialog's Class combo: Men/Air/Armored/...) resolve correctly per
// language — covered via triAssertLocalize rather than driving the
// full Insert Unit modal, which would need a map-click at an empty
// spot.

triSetLanguage "English"
triAssertEq [(triDisplay), 0]
// Verify editor button is present on the main menu and that cancelling the
// island-select dialog returns to the main menu (absorbed from open_island_select).
triAssert [(triGetControlVisible 115)]
triClick 115
triAssertEq [(triDisplay), 51]
triClick 2
triAssertEq [(triDisplay), 0]
triClick 115
triAssertEq [(triDisplay), 51]
triClick 1
triAssertEq [(triDisplay), 26]
// Verify editor controls are present (absorbed from enter_editor).
triAssert [(triGetControlVisible 51)]
triAssert [(triGetControlVisible 101)]
triAssert [(triGetControlVisible 102)]
triAssert [(triGetControlVisible 112)]
triScreenshot "00_editor_english"

// --- English baseline ------------------------------------------------
triAssertEq [(triControlText 101), "Load"]
triAssertEq [(triControlText 102), "Save"]
triAssertEq [(triControlText 103), "Clear"]
triAssertEq [(triControlText 106), "Merge"]
triAssertEq [(triControlText 107), "Preview"]
triAssertEq [(triControlText 108), "Continue"]
triAssertEq [(triControlText 112), "Show Textures"]
triAssertEq [(triControlText 2), "Exit"]
// ToolboxMode (idc 104) cells are rendered by CToolBox and are not exposed
// reliably through triVisibleTexts on this harness; screenshots cover the
// localized cell rendering while direct control/text checks cover buttons and
// programmatic combo rows below.
// Section combo (idc 109) row text is not exposed reliably through
// triVisibleTexts unless the drop-down is open. The current selected row is
// still covered through the main visible "Mission"/"Mise" label.
triAssertIncludes [(triVisibleTexts), "Mission"]
// Insert Unit dialog's Class combo entries (data-only check).
triAssertEq [(localize "STR_DISP_ARCUNIT_CLASS_MEN"), "Men"]
triAssertEq [(localize "STR_DISP_ARCUNIT_CLASS_ARMORED"), "Armored"]
triAssertEq [(localize "STR_DISP_ARCUNIT_CLASS_AIR"), "Air"]

// --- Switch to Czech and assert refresh -----------------------------
triSetLanguage "Czech"
triWaitFrames 60
triScreenshot "01_editor_czech"

triAssertEq [(triControlText 101), "Načíst"]
triAssertEq [(triControlText 102), "Uložit"]
triAssertEq [(triControlText 103), "Smazat"]
triAssertEq [(triControlText 106), "Spojit"]
triAssertEq [(triControlText 107), "Zahrát si"]
triAssertEq [(triControlText 108), "Pokračovat"]
triAssertEq [(triControlText 2), "Opustit"]
triAssertEq [(triControlText 112), "Ukázat textury"]
triAssert [(triGetControlVisible 2)]
triAssertIncludes [(triVisibleTexts), "Mise"]
triAssertEq [(localize "STR_DISP_ARCUNIT_CLASS_MEN"), "Pěchota"]
triAssertEq [(localize "STR_DISP_ARCUNIT_CLASS_ARMORED"), "Obrněné"]
triAssertEq [(localize "STR_DISP_ARCUNIT_CLASS_AIR"), "Vzdušné"]

// --- Restore English & exit cleanly ---------------------------------
triSetLanguage "English"
triAssertEq [(triControlText 101), "Load"]

triClick 2
triAssertEq [(triDisplay), 203]
triClick 1
triAssertEq [(triDisplay), 0]
triEndTest
