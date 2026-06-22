// fizzy #24: the Game-settings Voice Language picker listed the full text-language
// set (8 entries), letting players pick a dubbing the game doesn't ship ("too
// wide"). It must offer only languages with voiceover present. The Remaster ships
// voiceover for English (the base voice) + Czech, so the list is exactly those two.
//
// LanguageRegistry now exposes a hasVoice-filtered subset and GamePage's Voice
// Language row uses it. This drives the row and asserts it cycles through EXACTLY
// two entries: the value changes on the first Right (English -> Čeština) and wraps
// back to the start on the second. The assertion is relative (changed, then
// wrapped) so it needs no hard-coded autonyms.
//
// Broken-state delta: with the full 8-language list, two Rights land on the third
// language, so v2 != v0 and the test FAILs (not_two_entries).

triSetLanguage "English"
triClickText "OPTIONS"
triClickText "Game"
triAssertEq [(triDisplay), 9099]
triSimFrames 5

// Row 0 is Text Language; one Down focuses row 1 = Voice Language (value ctrl 512).
triSendKey 81
triSimFrames 3
private _v0 = triControlText 512;

// Right cycles the slider: English -> Čeština.
triSendKey 79
triSimFrames 3
private _v1 = triControlText 512;

// Right again must WRAP to the start (only two entries), back to English.
triSendKey 79
triSimFrames 3
private _v2 = triControlText 512;

if (_v0 == _v1) exitWith { format ["FAIL:no_change v0='%1' v1='%2'", _v0, _v1] };
if (_v0 != _v2) exitWith { format ["FAIL:not_two_entries v0='%1' v1='%2' v2='%3'", _v0, _v1, _v2] };

triEndTest
