#version 450

layout(location = 0) out vec3 vColor;

layout(push_constant) uniform BootstrapConstants
{
    vec4 viewport;
    vec4 clearColor;
} pc;

layout(set = 0, binding = 0, std140) uniform FrameConstants
{
    mat4 view;
    mat4 projection;
    vec4 viewport;
    vec4 clipPlanes;
    vec4 worldRect;
    vec4 fogParams;
    vec4 fogColor;
} frame;

vec2 kPositions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 kColors[3] = vec3[](
    vec3(0.95, 0.18, 0.16),
    vec3(0.18, 0.75, 0.32),
    vec3(0.20, 0.42, 0.95)
);

void main()
{
    vec4 viewport = frame.viewport.z > 0.0 ? frame.viewport : pc.viewport;
    float aspectComp = viewport.w > 0.0 ? viewport.w / max(viewport.z, 1.0) : 1.0;
    vec2 pos = vec2(kPositions[gl_VertexIndex].x * aspectComp, kPositions[gl_VertexIndex].y);
    gl_Position = vec4(pos, 0.0, 1.0);
    vColor = kColors[gl_VertexIndex];
}
