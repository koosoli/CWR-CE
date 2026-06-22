// Regression for the `format` stack-buffer-overflow crash.
//
// StrFormat (GameStateExtWorld.cpp) used to write the substituted output into a
// fixed `char result[2048]` stack buffer with unbounded strcpy/writes and no
// bounds check, so any `format` whose output exceeded 2048 chars smashed the
// stack -> 0xC0000005 (a crash AND a security hole on attacker-controlled input).
//
// Method: build a 4096-char string and format it twice into one result (~8 KB,
// far past the old 2048 limit). On the broken code the game crashed (exit
// 0xC0000005) before reaching triEndTest; with the fix (growable std::string
// buffer) it survives, so simply reaching triEndTest is the assertion.
triAssertEq [(triDisplay), 0]
_s = "x";
for "_i" from 1 to 12 do { _s = _s + _s };   // 2^12 = 4096 chars
_r = format ["%1 | %2", _s, _s];             // ~8195 chars, well past the old 2048
triEndTest
