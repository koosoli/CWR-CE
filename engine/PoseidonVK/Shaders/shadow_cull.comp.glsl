#version 450

layout(local_size_x = 64) in;

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
layout(set = 0, binding = 1, std430) buffer ShadowCounts { uint counts[]; } shadowCounts;

struct IndirectCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int vertexOffset;
    uint firstInstance;
};
layout(set = 0, binding = 2, std430) buffer ShadowIndirect { IndirectCommand commands[]; } indirect;

layout(push_constant) uniform ShadowCullDispatch
{
    mat4 lightVP;
    uint instanceCount;
    uint batchCount;
    uint cascadeIndex;
    uint indirectStride;
    uint countStride;
} dispatchInfo;

bool visibleLightSphere(ShadowInstance instance)
{
    // The row norms of lightVP * world bound the homogeneous clip extent of
    // every point in the local-space sphere, including perspective w.
    mat4 clipFromLocal = dispatchInfo.lightVP * instance.world;
    vec4 clip = clipFromLocal * vec4(instance.localBoundsCenter.xyz, 1.0);
    float radius = instance.localBoundsCenter.w;
    float rx = radius * length(vec3(clipFromLocal[0][0], clipFromLocal[1][0], clipFromLocal[2][0]));
    float ry = radius * length(vec3(clipFromLocal[0][1], clipFromLocal[1][1], clipFromLocal[2][1]));
    float rz = radius * length(vec3(clipFromLocal[0][2], clipFromLocal[1][2], clipFromLocal[2][2]));
    float rw = radius * length(vec3(clipFromLocal[0][3], clipFromLocal[1][3], clipFromLocal[2][3]));
    return clip.x + rx >= -clip.w - rw && clip.x - rx <= clip.w + rw &&
           clip.y + ry >= -clip.w - rw && clip.y - ry <= clip.w + rw &&
           clip.z + rz >= -rw && clip.z - rz <= clip.w + rw;
}

void main()
{
    uint instanceIndex = gl_GlobalInvocationID.x;
    if (instanceIndex >= dispatchInfo.instanceCount)
        return;

    ShadowInstance instance = shadow.instances[instanceIndex];
    if (instance.indexCount == 0u || instance.batchIndex >= dispatchInfo.batchCount || !visibleLightSphere(instance))
        return;

    uint countIndex = dispatchInfo.cascadeIndex * dispatchInfo.countStride + instance.batchIndex;
    uint slot = atomicAdd(shadowCounts.counts[countIndex], 1u);
    if (slot >= instance.batchCapacity)
        return;

    uint outputIndex = dispatchInfo.cascadeIndex * dispatchInfo.indirectStride + instance.indirectOffset + slot;
    indirect.commands[outputIndex].indexCount = instance.indexCount;
    indirect.commands[outputIndex].instanceCount = 1u;
    indirect.commands[outputIndex].firstIndex = instance.firstIndex;
    indirect.commands[outputIndex].vertexOffset = 0;
    indirect.commands[outputIndex].firstInstance = instanceIndex;
}
