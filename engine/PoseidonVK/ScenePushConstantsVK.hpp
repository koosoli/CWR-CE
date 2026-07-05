#pragma once

#include <Poseidon/Graphics/Core/MatrixConversion.hpp>

#include <cstddef>
#include <cstdint>

namespace Poseidon::vk
{

// Per-draw world transform for the Vulkan scene pipeline. Pushed at offset 0
// of the scene pipeline's push-constant range. Sized to a single mat4 (64 B)
// so it stays well under the 128 B minimum guaranteed by Vulkan for graphics
// stages, leaving room for future per-draw flags without a layout migration.
struct alignas(16) ScenePushConstantsVK
{
    GfxMatrix world = {};
};

inline constexpr std::uint32_t kScenePushConstantsSize =
    static_cast<std::uint32_t>(sizeof(ScenePushConstantsVK));

inline ScenePushConstantsVK BuildScenePushConstants(const GfxMatrix& world) noexcept
{
    ScenePushConstantsVK constants;
    constants.world = world;
    return constants;
}

inline ScenePushConstantsVK BuildIdentityScenePushConstants() noexcept
{
    ScenePushConstantsVK constants;
    constants.world._11 = 1.0f;
    constants.world._22 = 1.0f;
    constants.world._33 = 1.0f;
    constants.world._44 = 1.0f;
    return constants;
}

static_assert(offsetof(ScenePushConstantsVK, world) == 0);
static_assert(sizeof(ScenePushConstantsVK) == 64);
static_assert(kScenePushConstantsSize == 64);
static_assert(sizeof(ScenePushConstantsVK) <= 128, "Scene push constants must stay under the 128 B minimum");

} // namespace Poseidon::vk
