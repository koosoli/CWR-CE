#include <PoseidonVK/BufferVK.hpp>

#include <cstring>

namespace Poseidon::vk
{

uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        const bool typeMatches = (typeFilter & (1u << i)) != 0;
        const bool flagsMatch = (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;
        if (typeMatches && flagsMatch)
            return i;
    }

    return kInvalidMemoryType;
}

VkResult CreateHostVisibleBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size,
                                 VkBufferUsageFlags usage, BufferVK& out)
{
    DestroyBuffer(device, out);
    out.size = size;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &out.buffer);
    if (result != VK_SUCCESS)
    {
        DestroyBuffer(device, out);
        return result;
    }

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device, out.buffer, &requirements);

    const uint32_t memoryType =
        FindMemoryType(physicalDevice, requirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (memoryType == kInvalidMemoryType)
    {
        DestroyBuffer(device, out);
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = requirements.size;
    allocateInfo.memoryTypeIndex = memoryType;

    result = vkAllocateMemory(device, &allocateInfo, nullptr, &out.memory);
    if (result != VK_SUCCESS)
    {
        DestroyBuffer(device, out);
        return result;
    }

    result = vkBindBufferMemory(device, out.buffer, out.memory, 0);
    if (result != VK_SUCCESS)
    {
        DestroyBuffer(device, out);
        return result;
    }

    result = vkMapMemory(device, out.memory, 0, size, 0, &out.mapped);
    if (result != VK_SUCCESS)
    {
        DestroyBuffer(device, out);
        return result;
    }

    return VK_SUCCESS;
}

void UploadMappedBuffer(const BufferVK& buffer, const void* data, std::size_t size)
{
    if (!buffer.mapped || !data || size == 0)
        return;

    std::memcpy(buffer.mapped, data, size);
}

void DestroyBuffer(VkDevice device, BufferVK& buffer)
{
    if (device && buffer.mapped)
    {
        vkUnmapMemory(device, buffer.memory);
        buffer.mapped = nullptr;
    }
    if (device && buffer.buffer)
    {
        vkDestroyBuffer(device, buffer.buffer, nullptr);
        buffer.buffer = VK_NULL_HANDLE;
    }
    if (device && buffer.memory)
    {
        vkFreeMemory(device, buffer.memory, nullptr);
        buffer.memory = VK_NULL_HANDLE;
    }
    buffer.size = 0;
}

} // namespace Poseidon::vk
