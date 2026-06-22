#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/World/Terrain/Occlusion.hpp>

using namespace Poseidon;

// Occlusion::TestSphereWSpace projects a flare/light sphere into the column-major occlusion
// buffer (_data[x*_h+y], dimensions _w x _h) and samples a clamped pixel flat_quadangle. A sphere
// sitting on the right or bottom screen edge projects to x==_w (or y==_h); the inclusive sample
// loop then reads Get(_w, yy) == _data[_w*_h + yy], one full column past the 64 KB buffer
// (a flare visibility test while browsing missions after Cistka faulted on the
// first unmapped byte past _data). The fix clamps xMax/yMax to _w-1/_h-1 instead of _w/_h.
//
// SampleCoverage is the extracted sample-region core, so the off-by-one is exercisable without a
// live GScene projection. Broken-state delta: SampleCoverage(_w-1, .., sizeX>=1, ..) reads column
// _w and SampleCoverage(.., _h-1, .., sizeY>=1) reads row _h — a heap-buffer-overflow read that
// ASan aborts on (linux-x64-clang-san). With the fix every sampled cell stays in-bounds.
TEST_CASE("Occlusion::SampleCoverage never samples past the buffer edge", "[World][Occlusion]")
{
    const int W = 256, H = 256;
    Occlusion occ(W, H);
    occ.Clear(); // fill every in-bounds cell to the far value -> coverage is deterministic (1.0)

    // interior region: fully inside, coverage is well-defined
    REQUIRE(occ.SampleCoverage(W / 2, H / 2, 2, 2, 0) == Catch::Approx(1.0f));

    // right edge: x==W-1 with sizeX>=1 makes the pre-fix xMax==W -> Get(W,yy) reads column W
    REQUIRE(occ.SampleCoverage(W - 1, H / 2, 3, 3, 0) == Catch::Approx(1.0f));

    // bottom edge: y==H-1 with sizeY>=1 makes the pre-fix yMax==H -> Get(xx,H) reads row H
    REQUIRE(occ.SampleCoverage(W / 2, H - 1, 3, 3, 0) == Catch::Approx(1.0f));

    // exact corner: both edges overrun at once
    REQUIRE(occ.SampleCoverage(W - 1, H - 1, 4, 4, 0) == Catch::Approx(1.0f));
}
