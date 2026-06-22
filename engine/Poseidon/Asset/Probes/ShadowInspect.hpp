#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Offline shadow-map validator: loads a P3D, renders its depth from a chosen sun
// using the pure ShadowMath kernel (no GL, no live engine), then samples the
// shadow factor over a ground grid. Validates the shadow math on real CWA
// geometry and pre-tunes the depth bias offline.

namespace Poseidon
{

struct ShadowInspectOptions
{
    float sunAzDeg = 315.0f;   // azimuth in degrees: 0 = +Z, increasing toward +X
    float sunElDeg = 35.0f;    // elevation above the horizon, clamped to [1, 89]
    int lodIndex = 0;          // caster LOD (clamped to the model's range)
    int shadowRes = 512;       // depth-map resolution (shadowRes x shadowRes)
    int groundGrid = 128;      // ground sample grid (groundGrid x groundGrid)
    float groundMargin = 2.0f; // ground half-extent = model xz half-extent * this
    float biasScale = 1.5f;    // multiplies the slope-scaled depth bias
    bool wantImages = true;    // fill the debug grayscale buffers
};

struct ShadowInspectResult
{
    bool ok = false;
    std::string error;

    int lodUsed = 0;
    float resolution = 0.0f; // the chosen LOD's resolution band
    int vertexCount = 0;
    int triangleCount = 0;   // after quads are split

    float modelMin[3] = {0.0f, 0.0f, 0.0f};
    float modelMax[3] = {0.0f, 0.0f, 0.0f};

    float shadowedFraction = 0.0f; // fraction of the ground grid in shadow

    int depthW = 0;
    int depthH = 0;
    std::vector<uint8_t> depthGray; // depth map, visualised (1 channel)

    int groundW = 0;
    int groundH = 0;
    std::vector<uint8_t> groundGray; // ground visibility: 255 lit, 0 shadowed
};

ShadowInspectResult InspectShadow(const std::string& p3dPath, const ShadowInspectOptions& opts);

} // namespace Poseidon
