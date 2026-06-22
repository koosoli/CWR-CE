// GameCursorOverlay unit tests.  The overlay itself encodes two
// bits of pure logic:
//   • null safety — a freshly-constructed overlay with a null World
//     should not crash when Draw is called (covers the early-out
//     path that runs before any world state is queried).
//   • ToggleLock no-op — the game cursor doesn't grab the OS pointer
//     (that's viewer-only); pin that we don't accidentally promote
//     it from the base ICursorOverlay default.
//
// Every other behaviour requires a live World with at least one
// Display / InGameUI attached — that's tri-test territory.  See
// viewer_cursor_lock.test.sqf for end-to-end coverage of the
// viewer overlay; the game-mode menu cursor is covered by every
// existing menu screenshot test (the cursor sprite shows up
// because it goes through this overlay).

#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Graphics/Cursor/GameCursorOverlay.hpp>

using Poseidon::GameCursorOverlay;

TEST_CASE("GameCursorOverlay: null world is safe", "[UI][Cursor]")
{
    GameCursorOverlay cur(nullptr);
    // Null engine + null world should be a no-op, not a crash.
    cur.Draw(nullptr);
    CHECK(cur.IsLocked() == false); // base ICursorOverlay default
    SUCCEED("did not crash");
}

TEST_CASE("GameCursorOverlay: ToggleLock no-op", "[UI][Cursor]")
{
    GameCursorOverlay cur(nullptr);
    cur.ToggleLock(nullptr);
    CHECK(cur.IsLocked() == false);
    cur.ToggleLock(nullptr);
    CHECK(cur.IsLocked() == false);
}
