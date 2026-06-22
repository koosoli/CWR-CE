#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <Poseidon/World/Simulation/RemoteInterp.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>

// RemoteInterp is the client-side anti-warp smoother for remote (server/AI)
// units. The faithful OFP behaviour snaps a remote unit straight to every
// network position update; under lag/throttling that reads as a teleport.
// These cases pin the contract that replaces the snap: a moving unit is *eased*
// toward each update over several frames, and only error too large to ease
// without an obvious slide is cut. Broken state (revert to snap-on-update):
// case "eases ... without jumping" instead sees the full 2 m delta applied in a
// single step and fails.

using namespace Poseidon;

namespace
{
const Matrix3& Ident()
{
    return M3IdentityP;
}
} // namespace

TEST_CASE("RemoteInterp with no target is an inert no-op", "[network][remote_interp]")
{
    RemoteInterp ri;
    RemoteInterpParams p;
    Vector3 cur(5, 0, 7);
    RemoteInterpResult r = ri.Step(1.0f / 30, cur, VZero, Ident(), p);
    REQUIRE_FALSE(r.active);
    REQUIRE_FALSE(ri.HasTarget());
    REQUIRE(r.position.Distance(cur) == Catch::Approx(0.0f));
}

TEST_CASE("RemoteInterp eases toward a moving target without jumping", "[network][remote_interp]")
{
    RemoteInterp ri;
    RemoteInterpParams p;
    const float dt = 1.0f / 30;

    Vector3 cur(0, 0, 0);
    // A 2 m positional error, stationary target: well within the snap threshold.
    ri.SetTarget(Vector3(2, 0, 0), VZero, Ident());

    RemoteInterpResult r = ri.Step(dt, cur, VZero, Ident(), p);

    REQUIRE(r.active);
    REQUIRE_FALSE(r.snapped);

    float moved = r.position.Distance(cur);
    float maxStep = p.easeSpeedAbs * dt; // ~0.1 m at rest, 30 Hz
    // It nudged toward the target ...
    REQUIRE(moved > 0.0f);
    REQUIRE(moved <= Catch::Approx(maxStep).margin(1e-4));
    // ... and pointedly did NOT snap the full 2 m (the broken-state delta).
    REQUIRE(moved < 0.5f);
    REQUIRE(ri.HasTarget());
}

TEST_CASE("RemoteInterp converges to a stationary target over time", "[network][remote_interp]")
{
    RemoteInterp ri;
    RemoteInterpParams p;
    const float dt = 1.0f / 30;
    const Vector3 target(3, 0, 0);

    ri.SetTarget(target, VZero, Ident());

    Vector3 cur(0, 0, 0);
    for (int i = 0; i < 600 && ri.HasTarget(); i++)
    {
        RemoteInterpResult r = ri.Step(dt, cur, VZero, Ident(), p);
        REQUIRE_FALSE(r.snapped);
        cur = r.position;
    }

    REQUIRE_FALSE(ri.HasTarget()); // correction completed
    REQUIRE(cur.Distance(target) <= Catch::Approx(p.convergePos).margin(1e-3));
}

TEST_CASE("RemoteInterp never overshoots a within-reach target", "[network][remote_interp]")
{
    RemoteInterp ri;
    RemoteInterpParams p;
    const float dt = 1.0f / 30;
    // Error smaller than one step (maxStep ~0.1 m at rest): lands exactly on it.
    ri.SetTarget(Vector3(0.05f, 0, 0), VZero, Ident());

    RemoteInterpResult r = ri.Step(dt, VZero, VZero, Ident(), p);

    REQUIRE(r.position.Distance(Vector3(0.05f, 0, 0)) == Catch::Approx(0.0f).margin(1e-5));
}

TEST_CASE("RemoteInterp cuts to target when the error is too large to ease", "[network][remote_interp]")
{
    RemoteInterp ri;
    RemoteInterpParams p;
    const Vector3 target(50, 0, 0); // > snapDistAbs (10 m) at rest
    ri.SetTarget(target, VZero, Ident());

    RemoteInterpResult r = ri.Step(1.0f / 30, VZero, VZero, Ident(), p);

    REQUIRE(r.active);
    REQUIRE(r.snapped);
    REQUIRE(r.position.Distance(target) == Catch::Approx(0.0f));
    REQUIRE_FALSE(ri.HasTarget()); // consumed by the cut
}

TEST_CASE("RemoteInterp dead-reckons the target forward by its speed", "[network][remote_interp]")
{
    RemoteInterp ri;
    RemoteInterpParams p;
    const float dt = 0.1f;
    // Target at origin, moving +x at 10 m/s. After one 0.1 s step it should be
    // tracked at ~ (1,0,0) even though no new packet arrived.
    ri.SetTarget(Vector3(0, 0, 0), Vector3(10, 0, 0), Ident());

    ri.Step(dt, VZero, Vector3(10, 0, 0), Ident(), p);

    REQUIRE(ri.TargetPosition().X() == Catch::Approx(1.0f).margin(1e-4));
}

TEST_CASE("RemoteInterp caps how fast eased velocity tracks the target", "[network][remote_interp]")
{
    RemoteInterp ri;
    RemoteInterpParams p;
    const float dt = 1.0f / 30;
    // Target speed 100 m/s, current speed 0, position basically matched: the
    // velocity must ramp in (capped), not jump to 100 in one frame.
    ri.SetTarget(Vector3(0.001f, 0, 0), Vector3(100, 0, 0), Ident());

    RemoteInterpResult r = ri.Step(dt, VZero, VZero, Ident(), p);

    float maxSpeedStep = p.maxSpeedChange * dt;
    REQUIRE(r.speed.Size() <= Catch::Approx(maxSpeedStep).margin(1e-3));
    REQUIRE(r.speed.Size() > 0.0f);
}

TEST_CASE("RemoteInterp Clear abandons a pending correction", "[network][remote_interp]")
{
    RemoteInterp ri;
    RemoteInterpParams p;
    ri.SetTarget(Vector3(4, 0, 0), VZero, Ident());
    REQUIRE(ri.HasTarget());

    ri.Clear();
    REQUIRE_FALSE(ri.HasTarget());

    RemoteInterpResult r = ri.Step(1.0f / 30, VZero, VZero, Ident(), p);
    REQUIRE_FALSE(r.active);
}
