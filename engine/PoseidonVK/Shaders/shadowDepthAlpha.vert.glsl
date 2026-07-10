#version 450

layout(push_constant) uniform ShadowPC
{
    mat4 lightVP;
} pc;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

layout(location = 0) out vec2 vUV;

void main()
{
    vUV = aUV;
    gl_Position = pc.lightVP * vec4(aPos, 1.0f);
}
