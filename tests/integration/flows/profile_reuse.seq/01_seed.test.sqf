// Phase 1 of a two-boot sequence sharing one user dir.
//
// First boot against a fresh shared user dir: startup creates and selects the
// default profile (no profiles existed yet). Then leave a stale PlayerName pref
// pointing at a profile that does not exist, so the next boot in the same dir
// must fall back to the existing profile rather than recreate the stale one.

triSetLanguage "English"
triAssert [(triPlayerName)]
triSetPlayerPref "GhostProfile"
triEndTest
