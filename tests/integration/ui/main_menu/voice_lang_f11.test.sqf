// Voice and text language selection stay independent across the shared
// SetSelectedVoiceLanguage and SetLanguage paths.
//   1. Voice language can advance through kLangRotation entries
//   2. Wrap-around back to English works
//   3. Setting voice language does NOT touch the text language
//   4. Setting text language does NOT touch the voice language

triAssertEq [(triDisplay), 0]

// Pin starting state so the test is deterministic regardless of
// saved prefs.
triSetVoiceLanguage "English";
triSetLanguage "English";

// English → Czech (kLangRotation[0] → [1])
triSetVoiceLanguage "Czech"
triAssertEq [(triVoiceLanguage), "Czech"]
triAssertEq [(triGetLanguage), "English"]   // text untouched

// Czech → French
triSetVoiceLanguage "French"
triAssertEq [(triVoiceLanguage), "French"]

// Walk through the rest of the rotation and verify wrap back to
// English.
triSetVoiceLanguage "German"
triSetVoiceLanguage "Italian"
triSetVoiceLanguage "Spanish"
triSetVoiceLanguage "Russian"
triSetVoiceLanguage "English"
triAssertEq [(triVoiceLanguage), "English"];

// Sanity: text language can be set independently and the voice
// stays at English.
triSetLanguage "Czech"
triAssertEq [(triGetLanguage), "Czech"]
triAssertEq [(triVoiceLanguage), "English"];

// Restore defaults so other tests in the same process aren't surprised.
triSetLanguage "English";

triEndTest
