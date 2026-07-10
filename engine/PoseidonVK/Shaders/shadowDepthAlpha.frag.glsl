#version 450

layout(location = 0) in vec2 vUV;

layout(set = 0, binding = 0) uniform sampler2D uTex;

void main()
{
    if (texture(uTex, vUV).a < 0.5f)
    {
        discard;
    }
}
