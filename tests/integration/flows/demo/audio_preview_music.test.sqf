triSimFrames 5
triSetLanguage "English"

triAssertEq [(triDisplay), 0]
triClickText "OPTIONS"
triAssertEq [(triDisplay), 9099]
triClickText "Audio"
triAssertIncludes [(triVisibleTexts), "Music"]

private _before = triCategoryPreviewRestarts;

// Exercise the same category-preview path used by the Audio page after row
// focus dwell. Strict mode fails this test if any configured demo preview
// sample is missing or resolved through the wrong package bank.
triAssertEq [(triStartCategoryPreview "music"), "OK"]
triAssertEq [(triStartCategoryPreview "effects"), "OK"]
triAssertEq [(triStartCategoryPreview "speech"), "OK"]
triSimFrames 30

triAssertGt [(triCategoryPreviewRestarts), _before]

triEndTest
