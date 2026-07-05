#pragma once

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

static_assert(offsetof(BootstrapPushConstantsVK, viewport) == 0);
static_assert(offsetof(BootstrapPushConstantsVK, clearColor) == 16);
static_assert(sizeof(BootstrapPushConstantsVK) == 32);
static_assert(sizeof(BootstrapPushConstantsVK) <= 128, "Bootstrap push constants should stay tiny");

} // namespace Poseidon::vk
