#version 450

layout(push_constant) uniform ShadowPC
{
    mat4 lightVP;
} pc;

layout(location = 0) in vec3 aPos;

void main()
{
    gl_Position = pc.lightVP * vec4(aPos, 1.0f);
}
