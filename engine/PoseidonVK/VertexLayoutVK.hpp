#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>

namespace Poseidon::vk
{

// Scene-mesh vertex input contract. This mirrors the GL33 `SVertex` layout
// (pos, norm, uv) consumed by `vsTransform`, so a future Vulkan scene pipeline
// reads the SAME byte buffer the GL33 backend uploads and the shader `in`
// declarations stay in lock-step with the live vertex input state.
//
//   location 0: vec3 pos    (R32G32B32_SFLOAT, offset 0)
//   location 1: vec3 normal (R32G32B32_SFLOAT, offset 12)
//   location 2: vec2 uv     (R32G32_SFLOAT,    offset 24)
//
// Total stride: 32 bytes.
inline constexpr uint32_t kSceneVertexLocationPosition = 0;
inline constexpr uint32_t kSceneVertexLocationNormal = 1;
inline constexpr uint32_t kSceneVertexLocationTexcoord = 2;
inline constexpr uint32_t kSceneVertexAttributeCount = 3;

inline constexpr uint32_t kSceneVertexBinding = 0;
inline constexpr VkDeviceSize kSceneVertexStride = 32;

inline constexpr VkDeviceSize kSceneVertexPositionOffset = 0;
inline constexpr VkDeviceSize kSceneVertexNormalOffset = 12;
inline constexpr VkDeviceSize kSceneVertexTexcoordOffset = 24;

inline constexpr VkFormat kSceneVertexPositionFormat = VK_FORMAT_R32G32B32_SFLOAT;
inline constexpr VkFormat kSceneVertexNormalFormat = VK_FORMAT_R32G32B32_SFLOAT;
inline constexpr VkFormat kSceneVertexTexcoordFormat = VK_FORMAT_R32G32_SFLOAT;

inline VkVertexInputBindingDescription MakeSceneVertexBindingDescription() noexcept
{
    VkVertexInputBindingDescription description{};
    description.binding = kSceneVertexBinding;
    description.stride = kSceneVertexStride;
    description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return description;
}

inline VkVertexInputAttributeDescription MakeSceneVertexPositionAttribute() noexcept
{
    VkVertexInputAttributeDescription attribute{};
    attribute.location = kSceneVertexLocationPosition;
    attribute.binding = kSceneVertexBinding;
    attribute.format = kSceneVertexPositionFormat;
    attribute.offset = kSceneVertexPositionOffset;
    return attribute;
}

inline VkVertexInputAttributeDescription MakeSceneVertexNormalAttribute() noexcept
{
    VkVertexInputAttributeDescription attribute{};
    attribute.location = kSceneVertexLocationNormal;
    attribute.binding = kSceneVertexBinding;
    attribute.format = kSceneVertexNormalFormat;
    attribute.offset = kSceneVertexNormalOffset;
    return attribute;
}

inline VkVertexInputAttributeDescription MakeSceneVertexTexcoordAttribute() noexcept
{
    VkVertexInputAttributeDescription attribute{};
    attribute.location = kSceneVertexLocationTexcoord;
    attribute.binding = kSceneVertexBinding;
    attribute.format = kSceneVertexTexcoordFormat;
    attribute.offset = kSceneVertexTexcoordOffset;
    return attribute;
}

inline std::array<VkVertexInputAttributeDescription, kSceneVertexAttributeCount>
MakeSceneVertexAttributeDescriptions() noexcept
{
    return {MakeSceneVertexPositionAttribute(),
            MakeSceneVertexNormalAttribute(),
            MakeSceneVertexTexcoordAttribute()};
}

static_assert(kSceneVertexPositionOffset + 12 == kSceneVertexNormalOffset);
static_assert(kSceneVertexNormalOffset + 12 == kSceneVertexTexcoordOffset);
static_assert(kSceneVertexTexcoordOffset + 8 == kSceneVertexStride);
static_assert(kSceneVertexAttributeCount == 3);

} // namespace Poseidon::vk
