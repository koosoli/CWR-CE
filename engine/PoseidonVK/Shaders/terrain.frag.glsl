#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vWorldNormal;
layout(location = 2) in vec2 vLandUv;
layout(location = 3) flat in uvec2 vLandCell;
layout(location = 4) flat in uint vLod;
layout(location = 0) out vec4 outColor;

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

// TerrainVK descriptor set.  Index high bit is the captured ClampU|ClampV
// transition bit; the lower 15 bits are the native TextureVK layer index.
layout(set = 1, binding = 0) uniform sampler2D heightmap;
layout(set = 1, binding = 1) uniform usampler2D indexMap;
layout(set = 1, binding = 2) uniform sampler2D jitterMap;
layout(set = 1, binding = 3, std140) uniform TerrainParams
{
    vec2 worldOrigin;
    float landGrid;
    float terrainGrid;
    uvec4 dimensions;
    vec4 water;
    vec4 wetness;
} terrain;
layout(set = 1, binding = 4) uniform sampler2D terrainLayers[];

// Required visual receiver set.  No fallback is legal for these resources:
// terrain self-shadow, detail/blend control and sky visibility are distinct
// from CSM and must be supplied together by the future terrain pass.
layout(set = 2, binding = 0) uniform sampler2D terrainSelfShadow;
layout(set = 2, binding = 1) uniform sampler2D terrainDetail;
layout(set = 2, binding = 2) uniform sampler2D terrainBlend;
layout(set = 2, binding = 3) uniform sampler2D terrainSkyVisibility;

const uint kTransitionBit = 0x8000u;
const uint kLayerMask = 0x7fffu;

// Fine heightfield sample used by the ceiling comparison.  This is kept in the
// fragment stage (rather than consuming the CDLOD-interpolated y) so distant
// node morphing cannot turn into patch-sized shadow flicker.
float HeightAt(vec2 worldXZ)
{
    vec2 texel = vec2(terrain.dimensions.xy - uvec2(1u));
    vec2 uv = (worldXZ - terrain.worldOrigin) / max(terrain.terrainGrid * texel, vec2(0.0001));
    return texture(heightmap, clamp(uv, vec2(0.0), vec2(1.0))).r;
}

vec4 SampleNativeLayer(uint layer, vec2 uv, bool transition)
{
    // TextureVK descriptors retain native sampler state.  The captured
    // transition bit carries legacy ClampU|ClampV behavior at the terrain-cell
    // seam; ordinary layers retain repeated local coordinates.
    vec2 layerUv = transition ? clamp(uv, vec2(0.0), vec2(1.0)) : fract(uv);
    return texture(terrainLayers[nonuniformEXT(layer)], layerUv);
}

float CascadeShadow(vec3 worldPosition)
{
    if (frame.shadowCtl.x <= 0.5 || frame.cascadeCtl.x < 0.5)
        return 1.0;
    float eyeDepth = dot(worldPosition, frame.camFwd.xyz);
    int count = int(frame.cascadeCtl.x);
    int cascade = count;
    for (int i = 0; i < 4 && i < count; ++i)
        if (eyeDepth <= frame.cascadeSplits[i]) { cascade = i; break; }
    if (cascade >= count)
        return 1.0;
    vec4 lightPos = frame.cascadeVP[cascade] * vec4(worldPosition, 1.0);
    vec3 p = lightPos.xyz / lightPos.w;
    vec2 uv = vec2(p.x * 0.5 + 0.5, 0.5 - p.y * 0.5);
    if (any(lessThanEqual(uv, vec2(0.0))) || any(greaterThanEqual(uv, vec2(1.0))) || p.z <= 0.0 || p.z >= 1.0)
        return 1.0;
    float bias = frame.cascadeCtl.z * float((cascade + 1) * (cascade + 1));
    return texture(shadowMap, vec4(uv, float(cascade), p.z - bias));
}

vec3 TerrainLighting(vec3 normal, float directVisibility, float skyVisibility)
{
    vec3 n = normalize(normal);
    // The sky-view mask attenuates only ambient light.  CSM and the terrain
    // heightfield sweep attenuate only the direct sun; local lights remain
    // independent receivers just as they do in the WGPU reference.
    vec3 light = vec3(0.35 * skyVisibility);
    if (frame.lightingParams.x > 0.5)
        light += vec3(max(dot(n, -normalize(frame.sunDirection.xyz)), 0.0) * 0.65 * directVisibility);
    int count = min(int(frame.lightingParams.y + 0.5), 8);
    for (int i = 0; i < count; ++i)
    {
        vec3 toLight = frame.localLightPosition[i].xyz - vWorldPos;
        float d2 = dot(toLight, toLight);
        float range2 = frame.localLightPosition[i].w * frame.localLightPosition[i].w * 100.0;
        if (d2 <= 0.0001 || d2 >= range2) continue;
        float attenuation = min(1.0, frame.localLightPosition[i].w * frame.localLightPosition[i].w / d2);
        float diffuse = max(dot(n, normalize(toLight)), 0.0);
        light += (frame.localLightAmbient[i].rgb + frame.localLightDiffuse[i].rgb * diffuse) * attenuation * frame.lightingParams.z;
    }
    return light;
}

void main()
{
    uint entry = texelFetch(indexMap, ivec2(vLandCell), 0).r;
    uint layer = entry & kLayerMask;
    if (layer >= terrain.dimensions.w)
        discard; // CPU validation makes this unreachable; never index undefined descriptors.
    bool transition = (entry & kTransitionBit) != 0u;
    vec2 jitter = texelFetch(jitterMap, ivec2(vLandCell), 0).rg * (1.0 / 10.0);
    vec2 layerUv = vLandUv + jitter;
    vec4 albedo = SampleNativeLayer(layer, layerUv, transition);

    // Detail and blend are separate visual resources, not guessed from layer
    // IDs. The blend map's alpha weights the doubled-detail legacy modulation.
    vec2 detailUv = vWorldPos.xz * (1.0 / max(terrain.landGrid, 0.001));
    vec4 detail = texture(terrainDetail, detailUv + jitter);
    vec4 blend = texture(terrainBlend, detailUv);
    albedo.rgb *= mix(vec3(1.0), clamp(detail.rgb * 2.0, vec3(0.0), vec3(2.0)), blend.a);

    // The self-shadow resource is world-aligned and deliberately has an
    // extended (normally 2x) grid.  It stores a ceiling rather than a baked
    // scalar: use the fine heightfield sample to keep the test independent of
    // CDLOD morphing, then compose terrain occlusion and CSM by max().
    vec2 maskDims = vec2(textureSize(terrainSelfShadow, 0));
    vec2 maskScale = maskDims / vec2(terrain.dimensions.xy);
    vec2 maskCoord = (vWorldPos.xz - terrain.worldOrigin) / terrain.terrainGrid * maskScale;
    vec4 shadowSample = texture(terrainSelfShadow, (maskCoord + vec2(0.5)) / maskDims);
    float fineHeight = HeightAt(vWorldPos.xz);
    float selfLit = smoothstep(shadowSample.r - shadowSample.g,
                               shadowSample.r + shadowSample.g + 0.001, fineHeight);
    float terrainOcclusion = clamp(shadowSample.b * (1.0 - selfLit), 0.0, 1.0);
    float csmOcclusion = 1.0 - CascadeShadow(vWorldPos);
    float directVisibility = 1.0 - max(csmOcclusion, terrainOcclusion);
    vec2 skyDims = vec2(textureSize(terrainSkyVisibility, 0));
    vec2 skyCoord = (vWorldPos.xz - terrain.worldOrigin) / terrain.terrainGrid;
    float skyVisibility = texture(terrainSkyVisibility, (skyCoord + vec2(0.5)) / skyDims).r;
    vec3 color = albedo.rgb * TerrainLighting(vWorldNormal, directVisibility, skyVisibility);

    // Wet band follows the captured sea-level parameters and is applied before
    // fog/HDR composition, preserving linear lighting for the world target.
    float wet = 1.0 - smoothstep(terrain.water.x, terrain.water.x + terrain.wetness.x, vWorldPos.y);
    color *= mix(vec3(1.0), vec3(terrain.wetness.y), wet);
    float distanceToCamera = length(vWorldPos - frame.camPos.xyz);
    float fog = frame.fogParams.w > 0.5 ? 1.0 - pow(clamp(distanceToCamera / max(frame.fogParams.y, 1.0), 0.0, 1.0), 3.0) : 1.0;
    outColor = vec4(mix(frame.fogColor.rgb, color, fog), albedo.a);
}
