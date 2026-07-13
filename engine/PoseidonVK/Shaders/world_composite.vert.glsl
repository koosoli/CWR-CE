#version 450

layout(location = 0) out vec2 vUv;
layout(location = 1) flat out vec3 vCentreWorldRay;

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

void main()
{
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    vec2 position = positions[gl_VertexIndex];
    // The world pass uses a negative-height viewport to retain the engine's
    // projection convention; sample its attachment with the matching Y order.
    vUv = position * 0.5 + 0.5;
    vec4 centreView = inverse(frame.projection) * vec4(0.0, 0.0, 0.0, 1.0);
    vCentreWorldRay = normalize(transpose(mat3(frame.view)) * normalize(centreView.xyz / centreView.w));
    gl_Position = vec4(position, 0.0, 1.0);
}
