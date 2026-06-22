#pragma once

#include <Poseidon/Foundation/Math/Math3D.hpp>

namespace Poseidon
{

struct Object2DQuadCorners
{
    int bottomLeft = 0;
    int bottomRight = 0;
    int topLeft = 0;
    int topRight = 0;
};

inline int NearestUnusedObject2DVertex(Vector3Val target, const Vector3* positions, const int* vertices, bool* used)
{
    float bestDist = 1e30f;
    int bestSlot = 0;
    for (int slot = 0; slot < 4; ++slot)
    {
        if (used[slot])
        {
            continue;
        }
        Vector3Val pos = positions[vertices[slot]];
        const float dx = pos.X() - target.X();
        const float dy = pos.Y() - target.Y();
        const float dist = dx * dx + dy * dy;
        if (dist < bestDist)
        {
            bestDist = dist;
            bestSlot = slot;
        }
    }
    used[bestSlot] = true;
    return vertices[bestSlot];
}

inline Object2DQuadCorners SelectObject2DQuadCorners(const Vector3* positions, const int* vertices)
{
    float minX = positions[vertices[0]].X();
    float maxX = minX;
    float minY = positions[vertices[0]].Y();
    float maxY = minY;

    for (int slot = 1; slot < 4; ++slot)
    {
        Vector3Val pos = positions[vertices[slot]];
        if (pos.X() < minX)
            minX = pos.X();
        if (pos.X() > maxX)
            maxX = pos.X();
        if (pos.Y() < minY)
            minY = pos.Y();
        if (pos.Y() > maxY)
            maxY = pos.Y();
    }

    bool used[4] = {};
    Object2DQuadCorners corners;
    corners.bottomLeft = NearestUnusedObject2DVertex(Vector3(minX, minY, 0), positions, vertices, used);
    corners.bottomRight = NearestUnusedObject2DVertex(Vector3(maxX, minY, 0), positions, vertices, used);
    corners.topLeft = NearestUnusedObject2DVertex(Vector3(minX, maxY, 0), positions, vertices, used);
    corners.topRight = NearestUnusedObject2DVertex(Vector3(maxX, maxY, 0), positions, vertices, used);
    return corners;
}

} // namespace Poseidon
