#version 450
#extension GL_ARB_shader_draw_parameters : require

layout(push_constant) uniform ShadowPC { mat4 lightVP; } pc;

struct ShadowInstance
{
    vec4 localBoundsCenter;
    mat4 world;
    uint batchIndex;
    uint indirectOffset;
    uint batchCapacity;
    uint firstIndex;
    uint indexCount;
    float alphaCutoff;
    uint padding[3];
};
layout(set = 0, binding = 0, std430) readonly buffer ShadowInstances { ShadowInstance instances[]; } shadow;
layout(location = 0) in vec3 aPos;

void main()
{
    ShadowInstance instance = shadow.instances[gl_BaseInstanceARB];
    gl_Position = pc.lightVP * instance.world * vec4(aPos, 1.0);
}
