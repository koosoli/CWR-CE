// Aspect handling is user-selectable again.  Adaptive exposes the HUD
// width limit; Original stretches the HUD to the full screen and greys
// the limit row because there is nothing to clamp.

#include "../../../helpers/options_preamble.sqf"
#include "../../../helpers/display_preamble.sqf"

if ((triControlText 540) != "Aspect Handling") exitWith { format ["FAIL:missing_aspect_handling actual=%1", triControlText 540] };
if ((triControlText 550) != "HUD Width Limit") exitWith { format ["FAIL:missing_hud_width_limit actual=%1", triControlText 550] };

if ((triControlText 541) != "Adaptive") exitWith { format ["FAIL:style_not_adaptive actual=%1", triControlText 541] };
if ((triControlText 551) != "21:9") exitWith { format ["FAIL:clamp_not_21x9 actual=%1", triControlText 551] };

triClick 549
triSimFrames 2
if ((triControlText 541) != "Original (stretched HUD)") exitWith {
    format ["FAIL:style_not_original actual=%1", triControlText 541]
};
triClick 559
triSimFrames 2
if ((triControlText 551) != "21:9") exitWith {
    format ["FAIL:clamp_changed_when_original actual=%1", triControlText 551]
};

triEndTest
