#pragma once

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>

namespace Poseidon::vk
{

inline constexpr uint32_t kInvalidMemoryType = UINT32_MAX;

struct BufferVK
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
    VkDeviceSize size = 0;
};

uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

VkResult CreateHostVisibleBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size,
                                 VkBufferUsageFlags usage, BufferVK& out);

void UploadMappedBuffer(const BufferVK& buffer, const void* data, std::size_t size);

void DestroyBuffer(VkDevice device, BufferVK& buffer);

} // namespace Poseidon::vk
