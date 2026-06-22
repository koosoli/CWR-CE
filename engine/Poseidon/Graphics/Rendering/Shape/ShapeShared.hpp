#pragma once

#include <Poseidon/Graphics/Rendering/Draw/SpecLods.hpp>
#include <cmath>


namespace Poseidon
{
inline bool IsSpec(float resolution, float spec)
{
    return fabs(resolution - spec) < spec * 1e-3;
}

static inline bool ResolGeometryOnly(float resolution)
{
    return (IsSpec(resolution, GEOMETRY_SPEC) || IsSpec(resolution, VIEW_GEOM_SPEC) ||
            IsSpec(resolution, FIRE_GEOM_SPEC) || IsSpec(resolution, VIEW_PILOT_GEOM_SPEC) ||
            IsSpec(resolution, VIEW_CARGO_GEOM_SPEC) || IsSpec(resolution, VIEW_COMMANDER_GEOM_SPEC) ||
            IsSpec(resolution, VIEW_GUNNER_GEOM_SPEC) || IsSpec(resolution, MEMORY_SPEC) ||
            IsSpec(resolution, LANDCONTACT_SPEC) || IsSpec(resolution, ROADWAY_SPEC) ||
            IsSpec(resolution, PATHS_SPEC) || IsSpec(resolution, HITPOINTS_SPEC));
}

struct SortVertex
{
    int vertex, point, prior;
};

inline int CmpSortVertex(const SortVertex* v0, const SortVertex* v1)
{
    int d;
    d = v0->prior - v1->prior;
    if (d)
    {
        return d;
    }
    d = v0->point - v1->point;
    return d;
}

} // namespace Poseidon
