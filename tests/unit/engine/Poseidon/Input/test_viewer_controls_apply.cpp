// ApplyViewerControls unit tests — pure-math step that takes a
// ViewerControls snapshot and a ViewerStateIO and produces the new
// state.  Verifies sign conventions, zoom direction, animation
// transport, and side-effect flag propagation.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <Poseidon/Input/ViewerControlsApply.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>

using Catch::Matchers::WithinAbs;

using namespace Poseidon;
namespace
{
ViewerStateIO MakeBaseState()
{
    ViewerStateIO s;
    // Camera looking down +Z, +X right, +Y up — engine convention.
    s.cameraDirection = Vector3(0, 0, 1);
    s.cameraAside = Vector3(1, 0, 0);
    s.cameraUp = Vector3(0, 1, 0);
    return s;
}
} // namespace

TEST_CASE("ApplyViewerControls: zero controls leave state unchanged", "[Input][ViewerControlsApply]")
{
    auto s0 = MakeBaseState();
    s0.objectPos = Vector3(1, 2, 3);
    s0.cameraPos = Vector3(4, 5, 6);
    s0.animPhase = 0.25f;
    s0.animSpeed = 1.0f;

    ViewerControls c; // all zero / false
    auto s1 = ApplyViewerControls(c, s0, 1.0f / 60.0f, 1.0f);

    CHECK_THAT(s1.objectPos.X(), WithinAbs(1.0f, 1e-6f));
    CHECK_THAT(s1.objectPos.Y(), WithinAbs(2.0f, 1e-6f));
    CHECK_THAT(s1.objectPos.Z(), WithinAbs(3.0f, 1e-6f));
    CHECK_THAT(s1.cameraPos.X(), WithinAbs(4.0f, 1e-6f));
    CHECK_THAT(s1.animPhase, WithinAbs(0.25f, 1e-6f));
    CHECK_THAT(s1.animSpeed, WithinAbs(1.0f, 1e-6f));
}

TEST_CASE("ApplyViewerControls: LMB drag right moves object +X (camera right)", "[Input][ViewerControlsApply]")
{
    auto s0 = MakeBaseState();
    ViewerControls c;
    c.translateX = 100.0f; // pixels of drag
    auto s1 = ApplyViewerControls(c, s0, 1.0f / 60.0f, 1.0f);
    // kTranslateSpeed = 0.01 in viewer.cpp, applied to cameraAside = +X.
    CHECK_THAT(s1.objectPos.X(), WithinAbs(1.0f, 1e-6f));
    CHECK_THAT(s1.objectPos.Y(), WithinAbs(0.0f, 1e-6f));
}

TEST_CASE("ApplyViewerControls: LMB drag down moves object -Y (screen Y inverted)", "[Input][ViewerControlsApply]")
{
    auto s0 = MakeBaseState();
    ViewerControls c;
    c.translateY = 100.0f; // dragging down on screen
    auto s1 = ApplyViewerControls(c, s0, 1.0f / 60.0f, 1.0f);
    // Screen Y grows down, world Y grows up — apply step flips the sign.
    CHECK_THAT(s1.objectPos.X(), WithinAbs(0.0f, 1e-6f));
    CHECK_THAT(s1.objectPos.Y(), WithinAbs(-1.0f, 1e-6f));
}

TEST_CASE("ApplyViewerControls: 1 zoom tick multiplies distance by kZoomPerTick", "[Input][ViewerControlsApply]")
{
    auto s0 = MakeBaseState();
    s0.objectPos = Vector3(0, 0, 0);
    s0.cameraPos = Vector3(0, 0, -10); // 10 m behind target
    ViewerControls c;
    c.zoom = 1.0f;
    auto s1 = ApplyViewerControls(c, s0, 1.0f / 60.0f, 1.0f);
    // 10 * 0.85 = 8.5 m — camera dollies toward target along its
    // existing offset axis (here -Z), so newPos.Z = -8.5.
    CHECK_THAT(s1.cameraPos.X(), WithinAbs(0.0f, 1e-5f));
    CHECK_THAT(s1.cameraPos.Y(), WithinAbs(0.0f, 1e-5f));
    CHECK_THAT(s1.cameraPos.Z(), WithinAbs(-8.5f, 1e-5f));
}

TEST_CASE("ApplyViewerControls: 5 zoom ticks compound (ratio^5), not linear", "[Input][ViewerControlsApply]")
{
    // The whole point of switching to exponential dolly: 5 fast scroll
    // events in one frame should not jump 5 * 0.5 = 2.5 m.  Instead
    // the distance multiplies by 0.85^5 ≈ 0.4437.
    auto s0 = MakeBaseState();
    s0.objectPos = Vector3(0, 0, 0);
    s0.cameraPos = Vector3(0, 0, -10);
    ViewerControls c;
    c.zoom = 5.0f;
    auto s1 = ApplyViewerControls(c, s0, 1.0f / 60.0f, 1.0f);
    // 10 * 0.85^5 = 4.437...
    CHECK_THAT(s1.cameraPos.Z(), WithinAbs(-4.43705f, 1e-4f));
    // Sanity: a linear `+0.5 m * 5 ticks` model would land at -7.5.
    // We must not be there.
    CHECK(s1.cameraPos.Z() < -4.0f);
    CHECK(s1.cameraPos.Z() > -5.0f);
}

TEST_CASE("ApplyViewerControls: zoom clamps to min/max distance", "[Input][ViewerControlsApply]")
{
    // Zoom-in floor: 0.10 m so we don't pierce the model.
    auto s0 = MakeBaseState();
    s0.objectPos = Vector3(0, 0, 0);
    s0.cameraPos = Vector3(0, 0, -0.5f);
    ViewerControls c;
    c.zoom = 100.0f; // way past the floor
    auto s1 = ApplyViewerControls(c, s0, 1.0f / 60.0f, 1.0f);
    CHECK_THAT(s1.cameraPos.Z(), WithinAbs(-0.10f, 1e-5f));

    // Zoom-out ceiling: 500 m.
    auto s2 = MakeBaseState();
    s2.objectPos = Vector3(0, 0, 0);
    s2.cameraPos = Vector3(0, 0, -100.0f);
    ViewerControls c2;
    c2.zoom = -100.0f; // way past the ceiling
    auto s3 = ApplyViewerControls(c2, s2, 1.0f / 60.0f, 1.0f);
    CHECK_THAT(s3.cameraPos.Z(), WithinAbs(-500.0f, 1e-5f));
}

TEST_CASE("ApplyViewerControls: zoom is a no-op when camera coincides with target", "[Input][ViewerControlsApply]")
{
    auto s0 = MakeBaseState();
    s0.objectPos = Vector3(1, 2, 3);
    s0.cameraPos = Vector3(1, 2, 3); // degenerate
    ViewerControls c;
    c.zoom = 5.0f;
    auto s1 = ApplyViewerControls(c, s0, 1.0f / 60.0f, 1.0f);
    // No direction to dolly along — leave the camera where it is
    // rather than producing NaN.
    CHECK_THAT(s1.cameraPos.X(), WithinAbs(1.0f, 1e-6f));
    CHECK_THAT(s1.cameraPos.Y(), WithinAbs(2.0f, 1e-6f));
    CHECK_THAT(s1.cameraPos.Z(), WithinAbs(3.0f, 1e-6f));
}

TEST_CASE("ApplyViewerControls: rotate flips orientation", "[Input][ViewerControlsApply]")
{
    auto s0 = MakeBaseState();
    s0.objectOrient = M3Identity;
    ViewerControls c;
    c.rotateX = 100.0f; // 100 px → head ≈ 0.5 rad
    auto s1 = ApplyViewerControls(c, s0, 1.0f / 60.0f, 1.0f);
    // Orientation should differ from identity — tight check would
    // depend on internal math; here we just assert non-identity.
    Vector3 oldDir = s0.objectOrient.Direction();
    Vector3 newDir = s1.objectOrient.Direction();
    bool changed = (oldDir.X() != newDir.X()) || (oldDir.Y() != newDir.Y()) || (oldDir.Z() != newDir.Z());
    CHECK(changed);
}

TEST_CASE("ApplyViewerControls: playPauseAnim toggles animSpeed", "[Input][ViewerControlsApply]")
{
    auto s = MakeBaseState();
    s.animSpeed = 0.0f;

    ViewerControls c;
    c.playPauseAnim = true;
    s = ApplyViewerControls(c, s, 1.0f / 60.0f, 0.75f);
    CHECK_THAT(s.animSpeed, WithinAbs(0.75f, 1e-6f)); // started → default speed

    s = ApplyViewerControls(c, s, 1.0f / 60.0f, 0.75f);
    CHECK_THAT(s.animSpeed, WithinAbs(0.0f, 1e-6f)); // toggled to paused
}

TEST_CASE("ApplyViewerControls: resetAnim sets phase 0, paused", "[Input][ViewerControlsApply]")
{
    auto s = MakeBaseState();
    s.animPhase = 0.6f;
    s.animSpeed = 0.4f;

    ViewerControls c;
    c.resetAnim = true;
    s = ApplyViewerControls(c, s, 1.0f / 60.0f, 1.0f);
    CHECK_THAT(s.animPhase, WithinAbs(0.0f, 1e-6f));
    CHECK_THAT(s.animSpeed, WithinAbs(0.0f, 1e-6f));
}

TEST_CASE("ApplyViewerControls: openAnim sets phase 1, paused", "[Input][ViewerControlsApply]")
{
    auto s = MakeBaseState();
    ViewerControls c;
    c.openAnim = true;
    s = ApplyViewerControls(c, s, 1.0f / 60.0f, 1.0f);
    CHECK_THAT(s.animPhase, WithinAbs(1.0f, 1e-6f));
    CHECK_THAT(s.animSpeed, WithinAbs(0.0f, 1e-6f));
}

TEST_CASE("ApplyViewerControls: animScrub advances phase by deltaT", "[Input][ViewerControlsApply]")
{
    auto s = MakeBaseState();
    s.animPhase = 0.10f;

    ViewerControls c;
    c.animScrub = 1.0f; // forward at full speed
    s = ApplyViewerControls(c, s, 0.05f, 1.0f);
    CHECK_THAT(s.animPhase, WithinAbs(0.15f, 1e-5f));

    // Wraps in [0,1).
    s.animPhase = 0.95f;
    c.animScrub = 1.0f;
    s = ApplyViewerControls(c, s, 0.10f, 1.0f);
    CHECK_THAT(s.animPhase, WithinAbs(0.05f, 1e-5f));
}

TEST_CASE("ApplyViewerControls: side-effect flags propagate", "[Input][ViewerControlsApply]")
{
    auto s = MakeBaseState();
    ViewerControls c;
    c.reloadTextures = true;
    auto out = ApplyViewerControls(c, s, 1.0f / 60.0f, 1.0f);
    CHECK(out.didReloadTextures == true);
    CHECK(out.didExitViewer == false);

    c.reloadTextures = false;
    c.exitViewer = true;
    out = ApplyViewerControls(c, s, 1.0f / 60.0f, 1.0f);
    CHECK(out.didReloadTextures == false);
    CHECK(out.didExitViewer == true);
}
