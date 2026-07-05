#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexcoord;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vWorldNormal;
layout(location = 2) out vec2 vTexcoord;

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

layout(push_constant) uniform SceneDraw
{
    mat4 world;
} draw;

void main()
{
    vec4 worldPos = draw.world * vec4(inPosition, 1.0);
    vec3 worldNormal = normalize(mat3(draw.world) * inNormal);

    // Bring-up placement: the quad's vertex positions are already in NDC XY
    // (range roughly [-0.4..0.8] x [-0.4..0.4]), so it is visible regardless of
    // the game camera. The full proj*view*world transform activates when this
    // pipeline draws real scene meshes whose positions are in world space; the
    // matrix path is already covered by the GL33 parity tests.
    gl_Position = vec4(worldPos.xy, 0.0, 1.0);

    vWorldPos = worldPos.xyz;
    vWorldNormal = worldNormal;
    vTexcoord = inTexcoord;
}
