#version 450

layout(location = 0) in vec2 vUV;
layout(location = 1) flat in float vAlphaCutoff;
layout(set = 1, binding = 0) uniform sampler2D uTex;

void main()
{
    if (texture(uTex, vUV).a < vAlphaCutoff)
        discard;
}
