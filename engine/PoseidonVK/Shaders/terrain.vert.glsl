#version 450

// CDLOD terrain vertex path.  Grid vertices are normalized node coordinates;
// the per-instance payload supplies node origin, scale, LOD and morph band.
layout(location = 0) in vec3 inGrid;
layout(location = 1) in vec3 inNode;
layout(location = 2) in uint inLod;
layout(location = 3) in vec2 inMorph;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vWorldNormal;
layout(location = 2) out vec2 vLandUv;
layout(location = 3) flat out uvec2 vLandCell;
layout(location = 4) flat out uint vLod;

// Matches FrameConstantsVK exactly through the members consumed by terrain.
// Binding 2 remains the normal Frame descriptor's CSM image, rather than a
// second incompatible terrain-only shadow contract.
layout(set = 0, binding = 0, std140) uniform FrameConstants
{
    mat4 view;
    mat4 projection;
    mat4 sunMatrix;
    vec4 viewport;
    vec4 clipPlanes;
    vec4 worldRect;
    vec4 fogParams;
    vec4 fogColor;
    vec4 lightingParams;
    vec4 sunDirection;
    vec4 localLightPosition[8];
    vec4 localLightDiffuse[8];
    vec4 localLightAmbient[8];
    vec4 localLightDirection[8];
    vec4 grassParams;
    vec4 time;
    vec4 shadowCtl;
    mat4 cascadeVP[4];
    vec4 cascadeSplits;
    vec4 cascadeCtl;
    vec4 camFwd;
    vec4 camPos;
    vec4 specularColor;
    vec4 specularCtrl;
    vec4 cloudOrigin;
    vec4 wind;
    vec4 windOffset;
} frame;
layout(set = 0, binding = 2) uniform sampler2DArrayShadow shadowMap;

layout(set = 1, binding = 0) uniform sampler2D heightmap;
layout(set = 1, binding = 1) uniform usampler2D indexMap;
layout(set = 1, binding = 2) uniform sampler2D jitterMap;
layout(set = 1, binding = 3, std140) uniform TerrainParams
{
    vec2 worldOrigin;
    float landGrid;
    float terrainGrid;
    uvec4 dimensions; // height width, height height, land range, layer count
    vec4 water;       // sea level, time, swash speed, swash amplitude
    vec4 wetness;
} terrain;

float HeightAt(vec2 worldXZ)
{
    vec2 texel = vec2(terrain.dimensions.xy - uvec2(1u));
    vec2 uv = (worldXZ - terrain.worldOrigin) / max(terrain.terrainGrid * texel, vec2(0.0001));
    return texture(heightmap, clamp(uv, vec2(0.0), vec2(1.0))).r;
}

void main()
{
    vec2 nodeXZ = inNode.xy + inGrid.xy * inNode.z;
    // Parent-grid snapping is identical for all vertices in a node. Morphing
    // toward it prevents CDLOD cracks while the node approaches its range.
    float parentStep = max(inNode.z / 16.0, terrain.terrainGrid);
    vec2 parentXZ = floor((nodeXZ - terrain.worldOrigin) / parentStep + 0.5) * parentStep + terrain.worldOrigin;
    float cameraDistance = length(nodeXZ - frame.camPos.xz);
    float morph = smoothstep(inMorph.x, max(inMorph.y, inMorph.x + 0.001), cameraDistance);
    vec2 worldXZ = mix(nodeXZ, parentXZ, morph);
    float height = HeightAt(worldXZ);
    // A duplicated boundary ring gets a conservative vertical skirt.  It is
    // below the node's maximum expected morph difference, never a visual sea
    // level substitute.
    height -= inGrid.z * max(inNode.z * 0.08, terrain.terrainGrid * 2.0);

    float hL = HeightAt(worldXZ - vec2(terrain.terrainGrid, 0.0));
    float hR = HeightAt(worldXZ + vec2(terrain.terrainGrid, 0.0));
    float hD = HeightAt(worldXZ - vec2(0.0, terrain.terrainGrid));
    float hU = HeightAt(worldXZ + vec2(0.0, terrain.terrainGrid));
    vec3 normal = normalize(vec3(hL - hR, 2.0 * terrain.terrainGrid, hD - hU));
    vec3 worldPos = vec3(worldXZ.x, height, worldXZ.y);
    gl_Position = frame.projection * frame.view * vec4(worldPos, 1.0);
    vWorldPos = worldPos;
    vWorldNormal = normal;
    // Keep this coordinate continuous into the fragment stage. The material
    // shader derives its local tile coordinate after interpolation so a
    // triangle crossing a land-cell border cannot interpolate two fractional
    // coordinates through the wrong side of the tile.
    vec2 land = (worldXZ - terrain.worldOrigin) / terrain.landGrid;
    vLandUv = land;
    vLandCell = uvec2(clamp(floor(land), vec2(0.0), vec2(terrain.dimensions.z - 1u)));
    vLod = inLod;
}
