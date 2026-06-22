#pragma once

#include <cstdint>
#include <vector>

// Pure geometry decomposition helpers for the frame layer.  No GL,
// no scene-graph dependencies.  Unit-testable in isolation.
//
// A separate header so the polygon-fan-to-triangle-list rule lives in
// one pure function pinned by a unit test, rather than each backend's
// draw path re-implementing the decomposition implicitly.


namespace Poseidon
{
namespace render::frame
{

// Number of triangles produced by triangulating an `n`-vertex
// convex polygon as a fan.  Returns 0 for n<3.
constexpr int FanTriangleCount(int n) noexcept
{
    return n < 3 ? 0 : (n - 2);
}

// Number of indices required for fan triangulation of an n-gon.
// Equivalent to 3 * FanTriangleCount(n).
constexpr int FanIndexCount(int n) noexcept
{
    return 3 * FanTriangleCount(n);
}

// Decompose an n-vertex fan starting at `baseVertex` into a triangle-
// list index sequence.  Returns the appended index count.  Caller
// pre-reserves capacity.
//
// Layout for n=4: (0, 1, 2, 0, 2, 3) — pivot vertex 0, then
// (k-1, k) pairs for k = 2 .. n-1.
inline int AppendFanIndices(std::vector<std::uint16_t>& out, int n, std::uint16_t baseVertex = 0)
{
    const int tris = FanTriangleCount(n);
    out.reserve(out.size() + static_cast<size_t>(3 * tris));
    for (int k = 2; k < n; ++k)
    {
        out.push_back(static_cast<std::uint16_t>(baseVertex + 0));
        out.push_back(static_cast<std::uint16_t>(baseVertex + (k - 1)));
        out.push_back(static_cast<std::uint16_t>(baseVertex + k));
    }
    return 3 * tris;
}

} // namespace render::frame

} // namespace Poseidon
