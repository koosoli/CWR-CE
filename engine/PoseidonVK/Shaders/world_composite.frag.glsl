#version 450

layout(set = 0, binding = 0) uniform sampler2D worldColor;
layout(set = 0, binding = 2) uniform sampler2D exposureHistory;

layout(set = 0, binding = 1, std140) uniform FrameConstants
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
} frame;

layout(push_constant) uniform WorldCompositeParams
{
    float exposure;
    uint hdrEnabled;
    uint exposureHistoryValid;
} composite;

layout(location = 0) in vec2 vUv;
layout(location = 1) flat in vec3 vCentreWorldRay;
layout(location = 0) out vec4 outColor;

// Same fixed filmic stock as the reference renderer.  The former ACES fit is
// intentionally gentle and was flattening the midtones relative to both the
// original game and the reference HDR path.
vec3 HablePartial(vec3 x)
{
    return ((x * (0.15 * x + 0.10 * 0.50) + 0.20 * 0.02) /
            (x * (0.15 * x + 0.50) + 0.20 * 0.30)) - (0.02 / 0.30);
}

vec3 HableFilmic(vec3 x)
{
    return HablePartial(x) / HablePartial(vec3(11.2));
}

void main()
{
    vec4 world = texture(worldColor, vUv);
    // Keep the existing cloud compositor exactly display-referred unless HDR is opted in.
    if (composite.hdrEnabled == 0u)
    {
        outColor = vec4(world.rgb, 1.0);
        return;
    }

    // Scene shaders store pow(linear, 1/1.5); procedural sky and clouds use
    // sqrt. Without source provenance, this inverse is the closest common fit.
    vec3 scene = pow(max(world.rgb, vec3(0.0)), vec3(1.5));

    vec2 texel = 1.0 / vec2(textureSize(worldColor, 0));
    float eyeExposure = composite.exposure;
    if (composite.exposureHistoryValid != 0u)
        eyeExposure = texture(exposureHistory, vec2(0.5)).r;
    else
    {
        vec3 sunRay = normalize(-frame.sunDirection.xyz);
        float sunInView = smoothstep(0.94, 0.9985, dot(vCentreWorldRay, sunRay));
        eyeExposure = mix(composite.exposure, composite.exposure * 0.30, sunInView);
    }

    // Fused bright-pass bloom. Two wider rings make the HDR sun radiance
    // visibly bleed beyond its disc without another intermediate texture.
    const vec2 offsets[8] = vec2[](vec2(-2.0, 0.0), vec2(2.0, 0.0), vec2(0.0, -2.0), vec2(0.0, 2.0),
                                   vec2(-1.5, -1.5), vec2(1.5, -1.5), vec2(-1.5, 1.5), vec2(1.5, 1.5));
    vec3 bloom = max(scene - vec3(0.55), vec3(0.0)) * 0.25;
    for (int i = 0; i < 8; ++i)
    {
        vec3 sampleScene = pow(max(texture(worldColor, vUv + offsets[i] * texel).rgb, vec3(0.0)), vec3(1.5));
        bloom += max(sampleScene - vec3(0.55), vec3(0.0)) * 0.09375;
    }
    const vec2 ringDirections[8] = vec2[](vec2(-1.0, 0.0), vec2(1.0, 0.0), vec2(0.0, -1.0), vec2(0.0, 1.0),
                                           vec2(-0.7071, -0.7071), vec2(0.7071, -0.7071),
                                           vec2(-0.7071, 0.7071), vec2(0.7071, 0.7071));
    for (int i = 0; i < 8; ++i)
    {
        vec3 nearRing = pow(max(texture(worldColor, vUv + ringDirections[i] * texel * 7.0).rgb, vec3(0.0)), vec3(1.5));
        vec3 farRing = pow(max(texture(worldColor, vUv + ringDirections[i] * texel * 22.0).rgb, vec3(0.0)), vec3(1.5));
        bloom += max(nearRing - vec3(0.55), vec3(0.0)) * 0.0675;
        bloom += max(farRing - vec3(0.55), vec3(0.0)) * 0.027;
    }

    vec3 exposed = (scene + bloom) * eyeExposure;
    // Reference-compatible filmic shoulder plus a restrained display-space
    // contrast grade.  Keep the grade here (after the curve), never in
    // terrain/material shaders, so black levels and texture albedo stay
    // consistent across every receiver.
    vec3 mapped = HableFilmic(exposed);
    // Midpoint between the former neutral Vulkan resolve and the reference
    // grade.  Keep this conservative; the filmic curve already supplies most
    // of the desired black/midtone separation.
    mapped = clamp((mapped - vec3(0.5)) * 1.04 + vec3(0.5), 0.0, 1.0);
    vec3 srgb = mix(mapped * 12.92, 1.055 * pow(mapped, vec3(1.0 / 2.4)) - 0.055,
                    step(vec3(0.0031308), mapped));
    // User-facing display gamma.  This is deliberately applied after the
    // linear-to-sRGB transfer: source textures, terrain blending, and the
    // HDR exposure contract stay untouched.  gamma=1.2 is a subtle lift over
    // the reference-like filmic grade, not a second tone mapper.
    srgb = pow(clamp(srgb, vec3(0.0), vec3(1.0)), vec3(1.0 / 1.2));
    // Always write alpha=1 to the swapchain — the present surface is opaque.
    // Forwarding world.a causes desktop bleed-through for any scene pixel that
    // wrote alpha < 1 (e.g. terrain layer textures, sky, transparent geometry).
    outColor = vec4(srgb, 1.0);
}
