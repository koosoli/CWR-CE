// Hidden startup utility dialogs are still part of the startup flow even
// though the buttons are not visible on the Remaster root menu. Exercise
// the same DisplayMain::OnButtonClicked(IDC) path via triInvokeButton and
// assert the key-backed visible prompts on the Load / Save surfaces.

// Force English at start; the user's persistent config defaults to Czech
// on this machine and the first Load/Save text assertions below expect
// the English wording.  The test re-switches to Czech explicitly later
// to exercise the live-language path.
triSetLanguage "English"

triAssertEq [(triDisplay), 0]

triInvokeButton 121
triAssertEq [(triDisplay), 1]
triAssertEq [(localize "STR_DISP_MAIN_LOAD"), "Testing load"]
triAssertEq [(localize "STR_DISP_GAME_SELECT"), "Select the game:"]
triInvokeButton 2
triAssertEq [(triDisplay), 0]

triSetLanguage "Czech"
triWaitFrames 2

triInvokeButton 121
triAssertEq [(triDisplay), 1]
triAssertEq [(localize "STR_DISP_MAIN_LOAD"), "Testuji nahrávání"]
triAssertEq [(localize "STR_DISP_GAME_SELECT"), "Vyberte hru:"]
triInvokeButton 2
triAssertEq [(triDisplay), 0]

triInvokeButton 122
triAssertEq [(triDisplay), 13]
triAssertEq [(localize "STR_DISP_MAIN_SAVE"), "Testuji uložení"]
triAssertEq [(localize "STR_DISP_GAME_NAME"), "Jméno:"]
triInvokeButton 2
triAssertEq [(triDisplay), 0]

triEndTest
