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

layout(location = 0) out vec3 vWorldRay;
layout(location = 1) out vec2 vNdc;

void main()
{
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    vNdc = positions[gl_VertexIndex];
    gl_Position = vec4(vNdc, 0.0, 1.0);

    vec4 viewRay = inverse(frame.projection) * vec4(vNdc, 0.0, 1.0);
    vWorldRay = transpose(mat3(frame.view)) * normalize(viewRay.xyz / viewRay.w);
}
