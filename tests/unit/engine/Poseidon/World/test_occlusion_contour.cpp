#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Graphics/Rendering/Primitives/Poly.hpp>

using namespace Poseidon;

// Occlusion::RenderComponent builds an occluder's silhouette by feeding border edges into a
// stack-allocated BuildPoly, whose vertices live in the fixed _vertex[MaxPoly] buffer. An
// oversized/degenerate occluder — e.g. a huge addon model walked into from the inside
// (an 812 KB p3d) — yields a contour longer than MaxPoly.
//
// Before the fix BuildPoly::AddEdge always grew _n and wrote _vertex[_n], guarded only by a
// release-noop PoseidonAssert(_n < MaxPoly); once the contour filled, the insert loop walked
// past _vertex[MaxPoly] and smashed the frame — the corrupted spilled `transform` pointer then
// faulted in Vector3P::SetFastTransform. The fix caps the contour: AddEdge refuses once full,
// so a pathological occluder is merely truncated (slightly less culling), never corruption.
//
// Broken-state delta: AddEdge never returns false; N() climbs past MaxPoly and _vertex[192] is
// written one past the 192-element buffer (a stack-buffer-overflow under ASan).
TEST_CASE("BuildPoly::AddEdge caps the occluder contour at MaxPoly", "[World][Occlusion][Poly]")
{
    BuildPoly contour;
    contour.Init();

    // A chain of edges (0,1),(1,2),(2,3),... appends one fresh vertex to the tail each time,
    // driving the contour well past MaxPoly.
    bool refusedWhenFull = false;
    for (int i = 0; i < MaxPoly * 2; i++)
    {
        bool added = contour.AddEdge(i, i + 1);
        REQUIRE(contour.N() <= MaxPoly); // must never overflow _vertex[MaxPoly]
        if (!added)
        {
            refusedWhenFull = true;
        }
    }

    REQUIRE(refusedWhenFull);        // AddEdge must refuse once the contour is full
    REQUIRE(contour.N() <= MaxPoly); // and the final count stays bounded
}
