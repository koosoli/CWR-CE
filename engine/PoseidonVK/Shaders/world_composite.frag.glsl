#version 450

layout(set = 0, binding = 0) uniform sampler2D worldColor;

layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;

void main()
{
    // The world target is already display-referred UNORM output.
    outColor = texture(worldColor, vUv);
}
