// Options shell: baseline screenshot and keypad-Enter as modal default button.
// Keypad Enter follows the same default-button path as main Return, so
// SDL_SCANCODE_KP_ENTER (88) dismisses the Cancel-default confirm modal.

triSetLanguage "English"
triAssertEq [(triDisplay), 0]

triClickText "OPTIONS"
triAssertEq [(triDisplay), 9099]
triScreenshot "01_options_shell_index"

triClickText "Controls"
triWaitFrames 5

triClickText "Reset all to defaults"
triAssertIncludes [(triVisibleTexts), "Cancel"]
triScreenshot "02_confirm_modal"

triSendKey 88
triAssertExcludes [(triVisibleTexts), "Cancel"]
triAssertIncludes [(triVisibleTexts), "Keyboard & Mouse"]
triScreenshot "03_modal_closed_with_keypad_enter"

triEndTest
