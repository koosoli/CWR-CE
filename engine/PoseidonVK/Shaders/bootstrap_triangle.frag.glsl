#version 450

layout(location = 0) in vec3 vColor;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform BootstrapConstants
{
    vec4 viewport;
    vec4 clearColor;
} pc;

void main()
{
    outColor = vec4(mix(vColor, pc.clearColor.rgb, 0.12), 1.0);
}
