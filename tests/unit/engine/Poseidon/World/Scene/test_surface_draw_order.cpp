#include <catch2/catch_test_macros.hpp>

#include <Poseidon/World/Scene/SurfaceDrawOrder.hpp>

// The on-surface draw pass (roads + tyre tracks + marks + craters) must order
// road-before-decal by PassOrder, NEVER by camera distance.  A fresh transient
// decal carries distance2~=0 (SetAutoCenter(false)); under a plain
// back-to-front distance sort it ties or beats the road tile under the vehicle
// (also ~0) and the road repaints over it (asphalt ~93% alpha hides it).  These
// tests pin the pure ordering helper so that regression can't return silently.

using Poseidon::SurfaceDraw::CompareSurfaceDraw;
using Poseidon::SurfaceDraw::SurfaceDrawKey;
using Poseidon::SurfaceDraw::SurfaceOrder;
using Poseidon::SurfaceDraw::SurfacePassOrder;

namespace
{
// Two distinct, stable shape identities for grouping tests.
int shapeA = 0;
int shapeB = 0;
const void* kShapeA = &shapeA;
const void* kShapeB = &shapeB;
} // namespace

TEST_CASE("SurfacePassOrder: roads before decals", "[scene][surface]")
{
    REQUIRE(SurfacePassOrder(/*isTransientDecal=*/false) == static_cast<int>(SurfaceOrder::Road));
    REQUIRE(SurfacePassOrder(/*isTransientDecal=*/true) == static_cast<int>(SurfaceOrder::Decal));
    REQUIRE(SurfacePassOrder(false) < SurfacePassOrder(true));
}

TEST_CASE("CompareSurfaceDraw: PassOrder dominates distance (the regression)", "[scene][surface]")
{
    // The bug shape: road tile far from camera (large distance2), decal right
    // under the vehicle (distance2 ~= 0).  A distance-only back-to-front sort
    // would draw the near decal LAST... but the under-vehicle road tile is also
    // ~0, so the tie is unstable and the road can win.  PassOrder must make the
    // road draw first regardless of distance, so the decal lands on top.
    const SurfaceDrawKey road{SurfacePassOrder(false), kShapeA, /*distance2=*/12345.0f};
    const SurfaceDrawKey decal{SurfacePassOrder(true), kShapeB, /*distance2=*/0.0f};

    REQUIRE(CompareSurfaceDraw(road, decal) < 0); // road before decal
    REQUIRE(CompareSurfaceDraw(decal, road) > 0); // decal after road

    // Even when the road is the NEAREST thing (distance2 == 0, the under-vehicle
    // tile) and the decal is far, PassOrder still puts the road first.  A
    // distance-only back-to-front sort would draw the far decal first here.
    const SurfaceDrawKey nearRoad{SurfacePassOrder(false), kShapeA, 0.0f};
    const SurfaceDrawKey farDecal{SurfacePassOrder(true), kShapeB, 9999.0f};
    REQUIRE(CompareSurfaceDraw(nearRoad, farDecal) < 0);

    // The exact bug: the road tile under the vehicle and the fresh decal both
    // have distance2 == 0.  A distance comparator returns 0 (unstable tie) and
    // the road can repaint over the decal; PassOrder breaks it deterministically
    // in the decal's favour — road strictly first, never equal.
    const SurfaceDrawKey tiedRoad{SurfacePassOrder(false), kShapeA, 0.0f};
    const SurfaceDrawKey tiedDecal{SurfacePassOrder(true), kShapeB, 0.0f};
    REQUIRE(CompareSurfaceDraw(tiedRoad, tiedDecal) < 0);
    REQUIRE(CompareSurfaceDraw(tiedRoad, tiedDecal) != 0);
}

TEST_CASE("CompareSurfaceDraw: same PassOrder groups by shape", "[scene][surface]")
{
    const SurfaceDrawKey a{SurfacePassOrder(false), kShapeA, 100.0f};
    const SurfaceDrawKey b{SurfacePassOrder(false), kShapeB, 100.0f};
    // Different shapes with equal order/distance sort deterministically by shape
    // identity (grouping for state batching) — opposite signs, never 0.
    REQUIRE(CompareSurfaceDraw(a, b) == -CompareSurfaceDraw(b, a));
    REQUIRE(CompareSurfaceDraw(a, b) != 0);
}

TEST_CASE("CompareSurfaceDraw: same shape sorts back-to-front", "[scene][surface]")
{
    const SurfaceDrawKey near_{SurfacePassOrder(true), kShapeA, 10.0f};
    const SurfaceDrawKey far_{SurfacePassOrder(true), kShapeA, 500.0f};
    // Back-to-front: the farther object draws first (negative when comparing
    // far vs near).
    REQUIRE(CompareSurfaceDraw(far_, near_) < 0);
    REQUIRE(CompareSurfaceDraw(near_, far_) > 0);
}

TEST_CASE("CompareSurfaceDraw: identical keys compare equal", "[scene][surface]")
{
    const SurfaceDrawKey k{SurfacePassOrder(true), kShapeA, 42.0f};
    REQUIRE(CompareSurfaceDraw(k, k) == 0);
}
