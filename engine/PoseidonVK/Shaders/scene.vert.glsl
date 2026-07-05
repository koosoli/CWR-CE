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
    vec4 viewPos = frame.view * worldPos;
    gl_Position = frame.projection * viewPos;
    vWorldPos = worldPos.xyz;
    vWorldNormal = worldNormal;
    vTexcoord = inTexcoord;
}
