#pragma once

namespace Poseidon::SurfaceDraw
{

// Draw priority of a surface overlay inside the on-surface pass; lower draws
// first, so a decal always lies on top of its road regardless of distance.
enum class SurfaceOrder : int
{
    Road = 1,
    Decal = 2,
};

// Transient decals (tyre tracks, marks, craters) are TypeTempVehicle; every
// other on-surface object is a road or static surface overlay.
constexpr int SurfacePassOrder(bool isTransientDecal)
{
    return isTransientDecal ? static_cast<int>(SurfaceOrder::Decal) : static_cast<int>(SurfaceOrder::Road);
}

// Sort key for the on-surface draw queue.  POD; no engine dependencies.
struct SurfaceDrawKey
{
    int passOrder;     // SurfacePassOrder(...)  — primary key
    const void* shape; // LODShape identity — groups identical shapes (batching)
    float distance2;   // squared camera distance — back-to-front tiebreak only
};

// Strict-weak ordering for the on-surface queue: passOrder ascending, then
// shape grouping (state batching), then back-to-front within a shape.
// Road-vs-decal is keyed on passOrder, never distance: a fresh decal carries
// distance2~=0 (SetAutoCenter(false)) and would tie or beat the road tile
// under the vehicle in a distance sort, letting the road repaint over it.
inline int CompareSurfaceDraw(const SurfaceDrawKey& a, const SurfaceDrawKey& b)
{
    if (a.passOrder != b.passOrder)
    {
        return a.passOrder < b.passOrder ? -1 : 1;
    }
    if (a.shape != b.shape)
    {
        return a.shape < b.shape ? -1 : 1;
    }
    if (a.distance2 != b.distance2)
    {
        return a.distance2 > b.distance2 ? -1 : 1;
    }
    return 0;
}

} // namespace Poseidon::SurfaceDraw
