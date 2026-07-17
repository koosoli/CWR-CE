#version 450

// Heightfield self-shadow sweep.  The RGBA16F output is filterable and
// storage-writable: r = shadow-ceiling world height, g = penumbra half-width,
// b = strength.  This is deliberately a world-height ceiling rather than a
// terrain-only darkening factor, so all receivers can use the same resource.
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(set = 0, binding = 0, std140) uniform ShadowSweep
{
    vec2 worldOrigin;
    float terrainGrid;
    float penumbra;
    vec2 invScale;
    uint heightWidth;
    uint heightHeight;
    uint maskWidth;
    uint maskHeight;
    uint maxSteps;
    float strength;
    vec4 sunDirection; // xyz surface-to-light, w maximum terrain height
} sweep;
layout(set = 0, binding = 1) uniform sampler2D heightmap;
layout(set = 0, binding = 2, rgba16f) uniform writeonly image2D shadowMask;

const float kNoCeiling = -1.0e4;

float HeightLoad(ivec2 p)
{
    return texelFetch(heightmap, clamp(p, ivec2(0), ivec2(sweep.heightWidth, sweep.heightHeight) - ivec2(1)), 0).r;
}

float HeightBilinear(vec2 p)
{
    vec2 base = floor(p);
    vec2 f = p - base;
    ivec2 i = ivec2(base);
    return mix(mix(HeightLoad(i), HeightLoad(i + ivec2(1, 0)), f.x),
               mix(HeightLoad(i + ivec2(0, 1)), HeightLoad(i + ivec2(1, 1)), f.x), f.y);
}

void main()
{
    uvec2 id = gl_GlobalInvocationID.xy;
    if (id.x >= sweep.maskWidth || id.y >= sweep.maskHeight)
        return;
    vec2 cell0 = vec2(id) * sweep.invScale;
    float horizontal = length(sweep.sunDirection.xz);
    if (sweep.sunDirection.y <= 1.0e-3 || horizontal <= 1.0e-4)
    {
        imageStore(shadowMask, ivec2(id), vec4(kNoCeiling, 0.0, sweep.strength, 0.0));
        return;
    }
    vec2 direction = sweep.sunDirection.xz / horizontal;
    float slope = sweep.sunDirection.y / horizontal;
    float ceiling = kNoCeiling;
    float occluderHeight = 0.0;
    float occluderDistance = 0.0;
    for (uint step = 1u; step <= sweep.maxSteps; ++step)
    {
        float distance = float(step) * sweep.terrainGrid;
        if (sweep.sunDirection.w - distance * slope <= ceiling)
            break;
        float h = HeightBilinear(cell0 + direction * float(step));
        float candidate = h - distance * slope;
        if (candidate > ceiling)
        {
            ceiling = candidate;
            occluderHeight = h;
            occluderDistance = distance;
        }
    }
    float elevation = atan(slope);
    float litAngle = max(elevation - sweep.penumbra, 0.0);
    float darkAngle = min(elevation + sweep.penumbra, 1.5533);
    float high = occluderHeight - occluderDistance * tan(litAngle);
    float low = occluderHeight - occluderDistance * tan(darkAngle);
    imageStore(shadowMask, ivec2(id), vec4(ceiling, max((high - low) * 0.5, 0.0), sweep.strength, 0.0));
}
