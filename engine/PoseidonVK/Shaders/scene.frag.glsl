#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vWorldNormal;
layout(location = 2) in vec2 vTexcoord;
layout(location = 3) in float vFogFactor;

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
} frame;

void main()
{
    // Fixed directional light tilted from upper-left so the lit face shows a
    // clear gradient across the triangle. Normalized against the world normal
    // to produce a diffuse term without any texture or material binding yet.
    vec3 lightDir = normalize(vec3(-0.45f, -0.7f, -0.55f));
    float diffuse = max(dot(normalize(vWorldNormal), -lightDir), 0.0f);
    float ambient = 0.35f;
    float light = ambient + diffuse * 0.65f;

    // UV-driven two-tone color so the smoke test can tell the scene draw apart
    // from the bootstrap triangle and confirm UVs travel through correctly.
    vec3 baseColor = mix(vec3(0.10f, 0.55f, 0.85f), vec3(0.85f, 0.40f, 0.10f), vTexcoord.x);
    vec3 litColor = baseColor * light;

    // Fog driven from the uploaded frame constants: mix toward frame.fogColor
    // as distance grows. vFogFactor=1 near (no fog), 0 far (full fog), matching
    // the GL33 vsTransform/vsFog convention.
    vec3 fogged = mix(frame.fogColor.rgb, litColor, vFogFactor);
    outColor = vec4(fogged, 1.0f);
}
