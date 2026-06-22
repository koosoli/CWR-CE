#pragma once

#include <Poseidon/World/Terrain/Landscape.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Core/Config/EngineConfig.hpp>
#include <Random/randomGen.hpp>
#include <Poseidon/World/Scene/Scene.hpp>

#define WATER_TEX_SPEED 0.4f

namespace Poseidon
{
static const PreloadedShape CloudShapes[N_CLOUDS] = {Cloud2, Cloud3, Cloud1, Cloud4};

constexpr float maxTide = 5;
constexpr float maxWave = 0.25;

constexpr int LandSegmentSize = 8;
constexpr float InvLandSegmentSize = 1.0 / LandSegmentSize;

inline float MoveWater(float x, float z, const float coef = 1)
{
    float sinArg = Glob.time.toFloat() * WATER_TEX_SPEED * coef + x + z;
    x = fastFmod(sinArg, H_PI * 2);
    bool negative = false;
    if (x > H_PI)
    {
        x -= H_PI, negative = true;
    }
    if (x > H_PI * 0.5)
    {
        x = H_PI - x;
    }
    float xPow2 = x * x;
    float xPow3 = xPow2 * x;
    float xPow5 = xPow3 * xPow2;
    float sinVal = x - xPow3 * (1.0 / 2 / 3) + xPow5 * (1.0 / 2 / 3 / 4 / 5);
    if (negative)
    {
        sinVal = -sinVal;
    }
    AssertDebug(fabs(sinVal - sin(sinArg)) < 1e-2);
    return sinVal * 0.15;
}

inline RandomTable& GetSeedTable()
{
    static RandomTable SeedTable;
    return SeedTable;
}

inline int CalculateCacheSize(float viewDistance, float invLandGrid)
{
    float cacheSize = Square(viewDistance * invLandGrid * InvLandSegmentSize) * 10;
    int maxN = toIntCeil(cacheSize) + 8;
    if (maxN < 16)
    {
        maxN = 16;
    }
    const int maxReasonable = 512 * 512 / (LandSegmentSize * LandSegmentSize);
    PoseidonAssert(maxN < maxReasonable);
    if (maxN > maxReasonable)
    {
        maxN = maxReasonable;
    }
    return maxN;
}

}  // namespace Poseidon
