// Editor mission save -> list round-trip through the user-content folder.
//
// Built on the place_unit / preview_mission editor flow, but instead of previewing
// (Play) the mission we SAVE it and check it round-trips. Verifies the in-game
// editor WRITES and then LISTS a real (non-empty) user mission from the same
// on-disk folder (UserContentDir/missions/, which follows POSEIDON_USER_CONTENT_DIR).
// The companion Pester wrapper (tests/smoke/editor_mission_folder.tests.ps1) sets
// POSEIDON_USER_CONTENT_DIR to a temp dir and asserts the saved mission.sqm
// physically lands there.
//
// Flow:
//   main menu -> MISSION EDITOR (115) -> island select (51, default world, OK 1)
//   -> editor map (26) -> double-click the map (51) -> unit dialog (27), OK (1)
//   places a West soldier -> Save (102) -> dlg 29: type a name, OK (1) -> editor
//   -> Load (101) -> dlg 30: the name combo is rebuilt by scanning the missions
//   folder, so it must list the just-saved mission.
//
// What this scenario catches: the Load dialog rebuilds its name combo by scanning
// the missions folder on disk (not from the in-memory template), so if that scan
// path or the save break, the combo (IDC 101) comes up EMPTY and the
// triAssertControlText below reports FAIL:IDC 101 text='' instead of the name.
// It does NOT pin the folder LOCATION — save and load share one base, so the
// round-trip is self-consistent wherever it points. The companion Pester wrapper
// owns that half: it sets POSEIDON_USER_CONTENT_DIR and asserts the mission.sqm
// physically lands there (verified to fail when GetUserMissionsBase() is reverted
// to the roaming per-profile dir, while this scenario still exits 0).
//
// IDCs: 115 MISSION EDITOR, 51 island list / editor map, 27 unit dialog,
//       102 ARCMAP_SAVE -> dlg 29, 101 ARCMAP_LOAD -> dlg 30, 101 TEMPL_NAME,
//       1 OK, 2 cancel, 203 exit-confirm msgbox, 106 main-menu exit.

triSetLanguage "English"
triWaitFrames 5
triAssertEq [(triDisplay), 0]
triClick 115
triAssertEq [(triDisplay), 51]
triClick 1
triAssertEq [(triDisplay), 26]
triAssert [(triGetControlVisible 51)]

// Place a soldier: double-click the map opens the unit dialog; OK accepts the
// default West soldier, so we save a real mission rather than an empty template.
triDblClick 51
triAssertEq [(triDisplay), 27]
triClick 1
triAssertEq [(triDisplay), 26]

// Save it (instead of Preview/Play) under the editor's current world.
triClick 102
triAssertEq [(triDisplay), 29]
// Type the name through the REAL keyboard path: the field auto-focuses, and
// triTypeText routes the characters through World::DoChar -> the focused
// display's char dispatch, exactly as an SDL TEXT_INPUT keystroke does. This
// pins fizzy #179 ("editor mission-name field focuses but typed text never
// appears"): a focus or char-routing regression makes triAssertFocused or the
// text assert fail here. (triSendText [idc,...] would mask it — it calls OnChar
// directly on the control and never exercises focus or the dispatch chain.)
triAssert [(triGetControlFocused 101)]
triTypeText "TriEditorRoundtrip"
triAssertEq [(triControlText 101), "TriEditorRoundtrip"]
triScreenshot "00_save_name_entered"
triClick 1
triAssertEq [(triDisplay), 26]

// Open the Load dialog: its name combo scans the same missions folder. The
// freshly saved mission must be the (only) entry, selected at index 0.
triClick 101
triAssertEq [(triDisplay), 30]
triAssertEq [(triControlText 101), "TriEditorRoundtrip"]
triScreenshot "01_load_lists_saved_mission"

// Clean exit: cancel Load -> editor -> confirm exit -> main menu -> quit.
triClick 2
triAssertEq [(triDisplay), 26]
triClick 2
triAssertEq [(triDisplay), 203]
triClick 1
triAssertEq [(triDisplay), 0]
triClick 106
