// ViewerCursorOverlay unit tests — exercise the overlay's pure
// state-machine bits (the lock toggle).  Draw() touches the engine
// and is covered by the trident integration tests; here we just pin
// the behaviour that the overlay *owns its own* lock state and can be
// driven without an Engine instance.

#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Graphics/Cursor/ViewerCursorOverlay.hpp>

using Poseidon::ViewerCursorOverlay;

TEST_CASE("ViewerCursorOverlay: starts unlocked", "[UI][Cursor]")
{
    ViewerCursorOverlay cur;
    CHECK(cur.IsLocked() == false);
}

TEST_CASE("ViewerCursorOverlay: ToggleLock flips IsLocked", "[UI][Cursor]")
{
    ViewerCursorOverlay cur;
    // Pass nullptr for engine — ToggleLock is null-safe (it only
    // calls SetMouseGrab when engine != nullptr) and the bookkeeping
    // is what the help overlay reads.
    cur.ToggleLock(nullptr);
    CHECK(cur.IsLocked() == true);
    cur.ToggleLock(nullptr);
    CHECK(cur.IsLocked() == false);
    cur.ToggleLock(nullptr);
    CHECK(cur.IsLocked() == true);
}

TEST_CASE("ViewerCursorOverlay: each instance has independent state", "[UI][Cursor]")
{
    ViewerCursorOverlay a;
    ViewerCursorOverlay b;
    a.ToggleLock(nullptr);
    CHECK(a.IsLocked() == true);
    CHECK(b.IsLocked() == false);
    // Regression guard for the previous implementation that used a
    // function-static `bool s_grabbed` — two viewer instances would
    // have shared one global lock state, which broke the moment we
    // tried to write a test that drove ToggleLock twice.
}
