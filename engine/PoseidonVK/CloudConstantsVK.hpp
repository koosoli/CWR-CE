#pragma once

#include <PoseidonVK/FrameConstantsVK.hpp>

#include <cstddef>
#include <cstdint>

namespace Poseidon::vk
{

// Per-cloud-frame state deliberately lives outside the backend-neutral frame.
// It records history needed by the Vulkan reconstruction pass without making
// Vulkan resource or temporal policy part of the shared renderer contract.
struct CloudConstantsVK
{
    GfxMatrix previousView = {};
    GfxMatrix previousProjection = {};
    float cameraPosition[4] = {};
    float previousCameraPosition[4] = {};
    float windOffset[4] = {};
    float previousWindOffset[4] = {};
    float volumeOrigin[4] = {};
    float renderSizeAndHistory[4] = {}; // low-res width, height, valid, frame index
};

static_assert(offsetof(CloudConstantsVK, previousView) == 0);
static_assert(offsetof(CloudConstantsVK, previousProjection) == 64);
static_assert(offsetof(CloudConstantsVK, cameraPosition) == 128);
static_assert(offsetof(CloudConstantsVK, previousCameraPosition) == 144);
static_assert(offsetof(CloudConstantsVK, windOffset) == 160);
static_assert(offsetof(CloudConstantsVK, previousWindOffset) == 176);
static_assert(offsetof(CloudConstantsVK, volumeOrigin) == 192);
static_assert(offsetof(CloudConstantsVK, renderSizeAndHistory) == 208);
static_assert(sizeof(CloudConstantsVK) == 224);

} // namespace Poseidon::vk
