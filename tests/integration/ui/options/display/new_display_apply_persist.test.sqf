// End-to-end Display persistence: open Display, change Window Mode
// via the UI, press Apply (the Action row), confirm Keep on the
// ConfirmRevertPage modal, exit.  The accompanying Pester wrapper
// (tests/smoke/display_apply_persist.tests.ps1) drives this scenario
// against an ephemeral POSEIDON_USER_DIR and asserts display.cfg on
// disk has windowMode=1 (Fullscreen).
//
// The harness boots windowed (tri injects --window), so the Window Mode
// dropdown starts on "Window". One Right cycles Window -> Fullscreen
// (the dropdown order is Window, Fullscreen, Exclusive Fullscreen, with
// windowMode integers 2, 1, 0). We change to Fullscreen (= 1), Apply, Keep,
// and verify that change round-trips to display.cfg.

triSimFrames 5;
triSetLanguage "English";
triSimFrames 5;
triClickText "OPTIONS";
triSimFrames 5;
triSendKey 81;
triSimFrames 1;
triSendKey 40;
triSimFrames 5;

// Focus starts on Monitor (row 0).  Down twice → Window Mode (row 1).
triSendKey 81;   // DOWN
triSimFrames 2;

// One Right cycles Window → Fullscreen (windowMode 2 → 1).
triSendKey 79;   // RIGHT
triSimFrames 2;

// Five Downs from WindowMode → Apply. MoveFocus does NOT skip disabled
// rows, so Resolution, Refresh, Display Style and Ultrawide Clamp still
// take one focus step.
//   WindowMode (1) → Resolution (2) → Refresh (3, disabled)
//                  → Display Style (4) → Ultrawide Clamp (5) → Apply (6)
triSendKey 81;   // DOWN  (Resolution)
triSimFrames 2;
triSendKey 81;   // DOWN  (Refresh — disabled but focusable)
triSimFrames 2;
triSendKey 81;   // DOWN  (Display Style)
triSimFrames 2;
triSendKey 81;   // DOWN  (Ultrawide Clamp)
triSimFrames 2;
triSendKey 81;   // DOWN  (Apply)
triSimFrames 2;

// Enter on Apply pushes ConfirmRevertPage.
triSendKey 40;   // SDL_SCANCODE_RETURN
triSimFrames 10;
triAssertIncludes [triVisibleTexts, "Keep these display settings?"];
triAssertIncludes [triVisibleTexts, "Keep"];
triAssertIncludes [triVisibleTexts, "Revert"];

// Modal up — focus is on Revert by design (safer default).  Move to Keep
// with Left, then Enter.  Layout: Keep (idc 9201) is btn0, Revert (9202)
// is btn1; ConfirmRevertPage's WrapFocus uses an AnyAxis cycle so Left
// from Revert wraps to Keep.
triSendKey 80;   // SDL_SCANCODE_LEFT
triSimFrames 2;
triSendKey 40;   // ENTER on Keep
triSimFrames 10;

triEndTest;
