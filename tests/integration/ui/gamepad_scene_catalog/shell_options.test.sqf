// Temporary controller scene catalog captures for shell/options surfaces.
// These are review artifacts: they prove the screens are reachable and assert
// the scene metadata and generated prompt contract.

triSetLanguage "English"
triAssertEq [(triDisplay), 0]
_scene = triGetControllerScene
if (_scene != "Menu") exitWith { format ["FAIL:main_scene=%1", _scene] }
_section = triGetControllerSection
if (_section != "FocusList") exitWith { format ["FAIL:main_section=%1", _section] }
_prompts = triGetControllerPrompts
if (_prompts != "A Select|B Back|Start Options") exitWith { format ["FAIL:main_prompts=%1", _prompts] }
triScreenshot "00_main_menu"

triClickText "OPTIONS"
triAssertEq [(triDisplay), 9099]
_scene = triGetControllerScene
if (_scene != "Menu") exitWith { format ["FAIL:options_scene=%1", _scene] }
_section = triGetControllerSection
if (_section != "FocusList") exitWith { format ["FAIL:options_section=%1", _section] }
_prompts = triGetControllerPrompts
if (_prompts != "A Select|B Back") exitWith { format ["FAIL:options_prompts=%1", _prompts] }
triAssertIncludes [(triVisibleTexts), "Controls"]
triScreenshot "01_options_shell"

triClickText "Controls"
triAssertIncludes [(triVisibleTexts), "Keyboard & Mouse"]
_scene = triGetControllerScene
if (_scene != "Menu") exitWith { format ["FAIL:controls_scene=%1", _scene] }
_prompts = triGetControllerPrompts
if (_prompts != "A Select|B Back") exitWith { format ["FAIL:controls_prompts=%1", _prompts] }
triScreenshot "02_controls_index"

triClickText "Gamepad"
triAssertIncludes [(triVisibleTexts), "Gamepad"]
_scene = triGetControllerScene
if (_scene != "Menu") exitWith { format ["FAIL:gamepad_scene=%1", _scene] }
_section = triGetControllerSection
if (_section != "Pager") exitWith { format ["FAIL:gamepad_section=%1", _section] }
_prompts = triGetControllerPrompts
if (_prompts != "A Select|B Back|LT/RT Page") exitWith { format ["FAIL:gamepad_prompts=%1", _prompts] }
triScreenshot "03_gamepad_bindings"

triClick 513
triAssertIncludes [(triVisibleTexts), "Press"]
_scene = triGetControllerScene
if (_scene != "Menu") exitWith { format ["FAIL:capture_scene=%1", _scene] }
_section = triGetControllerSection
if (_section != "Capture") exitWith { format ["FAIL:capture_section=%1", _section] }
triAssertIncludes [(triGetControllerPrompts), "A Capture"]
triScreenshot "04_gamepad_capture_listening"

triClickText "Cancel"
triAssertIncludes [(triVisibleTexts), "Gamepad"]
triScreenshot "05_gamepad_capture_cancelled"

triEndTest
