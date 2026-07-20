#version 450

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
} frame;

// The image is a persistent HDR equirectangular sky map.  It is regenerated
// only when the celestial/weather cache key changes, never for camera motion.
layout(set = 1, binding = 0) uniform sampler2D skyMap;

layout(location = 0) in vec3 vWorldRay;
layout(location = 0) out vec4 outColor;

void main()
{
    vec3 ray = normalize(vWorldRay);
    const float invTwoPi = 0.15915494309189535;
    vec2 uv = vec2(atan(ray.z, ray.x) * invTwoPi + 0.5,
                   acos(clamp(ray.y, -1.0, 1.0)) * (1.0 / 3.141592653589793));
    vec3 color = texture(skyMap, uv).rgb;

    // The cached map and water reflection sample the same ungraded sky source.
    // Keeping its radiance untouched avoids a sky-only gamma conversion before
    // the direct UNORM presentation path; the present compositor clamps only at
    // the final display boundary.
    outColor = vec4(max(color, vec3(0.0)), 1.0);
}
