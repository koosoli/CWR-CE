// Startup profile selection runs at boot and produces a usable profile; the
// game reaches the main menu with it.
//
// The Trident harness gives each test a fresh POSEIDON_USER_DIR, so this
// exercises the no-existing-profile path: ProfileService creates and selects a
// default profile at boot and the menu comes up with a non-empty player name.
// The existing-profile-preference policy (an existing profile must be selected
// rather than a fresh default created) needs a pre-seeded profile layout, which
// the fresh-temp-dir harness cannot provide; the ProfileService unit tests
// cover that by seeding arbitrary profile layouts through the injected
// boundaries.

triSetLanguage "English"
triAssert [(triPlayerName)]
triClickText "OPTIONS"
triAssertEq [(triDisplay), 9099]
triEndTest
