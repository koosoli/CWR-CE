#pragma once

#include <PoseidonVK/FrameConstantsVK.hpp>

#include <cstddef>
#include <cstdint>

namespace Poseidon::vk
{

struct alignas(16) BootstrapPushConstantsVK
{
    float viewport[4] = {};   // x, y, width, height
    float clearColor[4] = {}; // rgba
};

inline constexpr std::uint32_t kBootstrapPushConstantsSize =
    static_cast<std::uint32_t>(sizeof(BootstrapPushConstantsVK));

inline BootstrapPushConstantsVK BuildBootstrapPushConstants(
    int viewportWidth, int viewportHeight, const float clearColor[4]) noexcept
{
    BootstrapPushConstantsVK constants;
    constants.viewport[0] = 0.0f;
    constants.viewport[1] = 0.0f;
    constants.viewport[2] = static_cast<float>(viewportWidth);
    constants.viewport[3] = static_cast<float>(viewportHeight);
    constants.clearColor[0] = clearColor[0];
    constants.clearColor[1] = clearColor[1];
    constants.clearColor[2] = clearColor[2];
    constants.clearColor[3] = clearColor[3];
    return constants;
}

inline BootstrapPushConstantsVK BuildBootstrapPushConstants(
    const FrameConstantsVK& frameConstants, const float clearColor[4]) noexcept
{
    BootstrapPushConstantsVK constants;
    constants.viewport[0] = frameConstants.viewport[0];
    constants.viewport[1] = frameConstants.viewport[1];
    constants.viewport[2] = frameConstants.viewport[2];
    constants.viewport[3] = frameConstants.viewport[3];
    constants.clearColor[0] = clearColor[0];
    constants.clearColor[1] = clearColor[1];
    constants.clearColor[2] = clearColor[2];
    constants.clearColor[3] = clearColor[3];
    return constants;
}

static_assert(offsetof(BootstrapPushConstantsVK, viewport) == 0);
static_assert(offsetof(BootstrapPushConstantsVK, clearColor) == 16);
static_assert(sizeof(BootstrapPushConstantsVK) == 32);
static_assert(sizeof(BootstrapPushConstantsVK) <= 128, "Bootstrap push constants should stay tiny");

} // namespace Poseidon::vk
