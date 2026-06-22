// Phase 2 of the sequence — boots against the same user dir phase 1 left behind
// (the .seq runner shares POSEIDON_USER_DIR across phases).
//
// The PlayerName pref points at the missing "GhostProfile", but a usable profile
// already exists from phase 1, so startup must select that existing profile and
// must NOT recreate "GhostProfile". Before the fix, a stale pref was used
// verbatim and its directory recreated; now an existing profile is preferred.

triSetLanguage "English"
triAssert [(triPlayerName)]
triAssertProfileMissing "GhostProfile"
triEndTest
