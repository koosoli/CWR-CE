// Regression: the mission editor must stay on the shared software-cursor path
// used by the in-mission map/notebook stack.
//
// Before the active-display walk was unified, filtering disabled option/map
// displays at the top level hid the editor branch from both the harness and the
// cursor overlay. The editor no longer resolved as the active display, so
// cursor regressions slipped past tri tests.
//
// Method:
//   1. Open the mission editor from the main menu.
//   2. Move the synthetic cursor to a stable centre point on the editor.
//   3. Open a disabled child display over the editor.
//   4. Assert the live software-cursor draw follows that child display and
//      still moves on screen after a later cursor move.
//
// Broken state:
//   - triAssertDisplay 26 failed because the harness no longer saw the editor.
//   - disabled child displays under the editor were rejected by the shared
//     active-display walk because it looked at the deepest child's
//     IsDisplayEnabled() flag instead of the root editor branch.
//
// The cursor draw flat_quad is reported in live 2D pixels, so exact x/y/w/h values
// vary with the viewport (e.g. 40x40 at 800x600 vs ~67x67 at 1333x1000).  This
// test therefore treats the routed display id as the load-bearing assertion and
// only checks that the child-display move meaningfully changes the draw flat_quad.

triSetLanguage "English"
triWaitFrames 5
triAssertEq [(triDisplay), 0]
triClick 115
triAssertEq [(triDisplay), 51]
triClick 1
triAssertEq [(triDisplay), 26]
triAssertEq [(triCursorLocked), "unlocked"]
triWaitFrames 20
triScreenshot "00_editor_open"

triCursorMove [0.00, 0.00]
triAssertNear [(triCursorPos), "0.0,0.0", 0.001]
triWaitFrames 10
triScreenshot "01_cursor_center"
private _editorRect = parseSimpleArray format ["[%1]", triCursorDrawRect];
if (count _editorRect < 5) exitWith { format ["FAIL:bad_editor_flat_quad %1", triCursorDrawRect] };
if ((_editorRect select 0) != 26) exitWith { format ["FAIL:editor_cursor_idd %1", _editorRect] };
if ((_editorRect select 3) <= 0 || (_editorRect select 4) <= 0) exitWith {
    format ["FAIL:editor_cursor_size %1", _editorRect]
};

triOpenDisabledChildDisplay 99001
triWaitFrames 10
triAssertEq [(triDisplay), 99001]
triScreenshot "02_child_display_open"
private _childRect = parseSimpleArray format ["[%1]", triCursorDrawRect];
if (count _childRect < 5) exitWith { format ["FAIL:bad_child_flat_quad %1", triCursorDrawRect] };
if ((_childRect select 0) != 99001) exitWith { format ["FAIL:child_cursor_idd %1", _childRect] };

triCursorMove [0.70, 0.20]
triAssertNear [(triCursorPos), "0.7,0.2", 0.001]
triWaitFrames 10
triScreenshot "03_child_display_cursor_map"
private _childMovedRect = parseSimpleArray format ["[%1]", triCursorDrawRect];
if (count _childMovedRect < 5) exitWith { format ["FAIL:bad_moved_child_flat_quad %1", triCursorDrawRect] };
if ((_childMovedRect select 0) != 99001) exitWith { format ["FAIL:moved_child_cursor_idd %1", _childMovedRect] };
if ((_childMovedRect select 1) <= ((_childRect select 1) + 100)) exitWith {
    format ["FAIL:child_cursor_x_not_moved from=%1 to=%2", _childRect, _childMovedRect]
};
if ((_childMovedRect select 2) <= ((_childRect select 2) + 30)) exitWith {
    format ["FAIL:child_cursor_y_not_moved from=%1 to=%2", _childRect, _childMovedRect]
};
if ((_childMovedRect select 3) != (_childRect select 3) || (_childMovedRect select 4) != (_childRect select 4)) exitWith {
    format ["FAIL:child_cursor_size_changed from=%1 to=%2", _childRect, _childMovedRect]
};

triEndTest
