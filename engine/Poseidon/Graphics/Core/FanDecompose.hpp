#pragma once

#include <cstddef>

// Polygon-fan -> triangle-list index decomposition.
//
// Input is an `n`-vertex fan referencing source vertices via the `src` array
// (`n >= 3`).  Output is `(n - 2) * 3` triangle-list indices written into `dst`,
// each offset by `vertexBase` so the indices target a slot in a shared mesh
// buffer.  Pure free function — no GL, no engine state.

namespace Poseidon
{
namespace render::geom
{

// Index counts for a fan of `n` vertices.  n < 3 produces zero
// triangles (degenerate; callers should skip the draw).
constexpr int FanTriangleIndexCount(int n) noexcept
{
    return n < 3 ? 0 : (n - 2) * 3;
}

template <typename SrcIndex, typename DstIndex>
void FanToTriangles(const SrcIndex* src, int n, int vertexBase, DstIndex* dst) noexcept
{
    if (n < 3 || !src || !dst)
        return;

    const int base0 = static_cast<int>(src[0]) + vertexBase;
    int prev = static_cast<int>(src[1]) + vertexBase;

    for (int i = 2; i < n; ++i)
    {
        const int curr = static_cast<int>(src[i]) + vertexBase;
        dst[0] = static_cast<DstIndex>(base0);
        dst[1] = static_cast<DstIndex>(prev);
        dst[2] = static_cast<DstIndex>(curr);
        dst += 3;
        prev = curr;
    }
}

} // namespace render::geom

} // namespace Poseidon
