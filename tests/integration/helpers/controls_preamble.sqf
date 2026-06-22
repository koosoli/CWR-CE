// Navigate from the Options shell to the Controls page.
// Use after options_preamble (or whenever on the Options shell at IDD 9099).
triClickText "Controls"
triAssertIncludes [(triVisibleTexts), "Keyboard & Mouse"]
