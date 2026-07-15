// EngineVK_Shadow.cpp
// Cascade shadow-map implementation for PoseidonVK.
// Mirrors the GL33 path (EngineGL33_ShadowDepth.cpp) using Vulkan idioms:
//  - Depth-only 2D-array image (kShadowCascades layers)
//  - One VkRenderPass, per-layer VkFramebuffer
//  - Solid caster pipeline: vertex-only, push-constant lightVP
//  - Alpha caster pipeline: vertex + fragment (alpha-discard), same push constant
//  - One-shot command buffer per frame (submitted before the main CB)
//  - Shadow UBO fields filled in UpdateShadowFrameConstants -> UploadFrameConstants

#include <PoseidonVK/EngineVK.hpp>
#include <PoseidonVK/BufferVK.hpp>
#include <PoseidonVK/DescriptorBindingsVK.hpp>
#include <PoseidonVK/SceneDrawCommandsVK.hpp>
#include <PoseidonVK/TextureVK.hpp>
#include <PoseidonVK/VertexLayoutVK.hpp>
#include <Poseidon/Core/TaskPool.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Graphics/Shadow/ShadowMath.hpp>

#include <algorithm>
#include <cstring>
#include <iterator>
#include <string>
#include <vector>

namespace Poseidon
{
namespace
{
const char* VkResultName(VkResult result)
{
    switch (result)
    {
        case VK_SUCCESS:                    return "VK_SUCCESS";
        case VK_ERROR_OUT_OF_HOST_MEMORY:   return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_FEATURE_NOT_PRESENT:  return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_MEMORY_MAP_FAILED:    return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_LAYER_NOT_PRESENT:    return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:  return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_DEVICE_LOST:          return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_OUT_OF_DATE_KHR:      return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_OUT_OF_POOL_MEMORY:   return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_SUBOPTIMAL_KHR:             return "VK_SUBOPTIMAL_KHR";
        default:                            return "VkResult";
    }
}

bool RecreateSignaledShadowFence(VkDevice device, VkFence& fence)
{
    if (fence)
        vkDestroyFence(device, fence, nullptr);
    VkFenceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    return vkCreateFence(device, &info, nullptr, &fence) == VK_SUCCESS;
}
} // namespace
} // namespace Poseidon

// Shader sources embedded at compile time via GenerateShaderHeader.cmake
namespace
{
struct ShadowPushConstantsVK
{
    float lightVP[16] = {};
    // Affine scene transforms have a fixed final column.  Pack the three
    // variable columns plus translation so CSM alpha cutoff still fits Vulkan's
    // guaranteed 128-byte push-constant budget.
    float worldColumns[3][4] = {};
    float translation[3] = {};
    float alphaCutoff = 0.5f;
};

static_assert(sizeof(ShadowPushConstantsVK) == sizeof(float) * 32,
              "shadow push constants must match shadowDepth*.glsl");

constexpr const char kShadowDepthVertSrc[] =
#include <PoseidonVK/Shaders/shadowDepth.vert.glsl.hpp>
;

constexpr const char kShadowAlphaVertSrc[] =
#include <PoseidonVK/Shaders/shadowDepthAlpha.vert.glsl.hpp>
;

constexpr const char kShadowAlphaFragSrc[] =
#include <PoseidonVK/Shaders/shadowDepthAlpha.frag.glsl.hpp>
;

constexpr const char kShadowCullComputeSrc[] =
#include <PoseidonVK/Shaders/shadow_cull.comp.glsl.hpp>
;

constexpr const char kShadowGpuDepthVertSrc[] =
#include <PoseidonVK/Shaders/shadowDepthGpu.vert.glsl.hpp>
;

constexpr const char kShadowGpuAlphaVertSrc[] =
#include <PoseidonVK/Shaders/shadowDepthGpuAlpha.vert.glsl.hpp>
;

constexpr const char kShadowGpuAlphaFragSrc[] =
#include <PoseidonVK/Shaders/shadowDepthGpuAlpha.frag.glsl.hpp>
;
} // namespace

namespace Poseidon
{

// ---------------------------------------------------------------------------
// Public overrides -- shadow enable / tuning / sun factor
// ---------------------------------------------------------------------------

void EngineVK::SetShadowMapsEnabled(bool enabled)
{
    _shadowTuning.enabled = enabled;
}

bool EngineVK::ShadowMapsEnabled() const
{
    return _shadowTuning.enabled;
}

Engine::ShadowMapTuning EngineVK::GetShadowMapTuning() const
{
    return _shadowTuning;
}

void EngineVK::SetShadowMapTuning(const ShadowMapTuning& tuning)
{
    _shadowTuning = tuning;
}

void EngineVK::SetShadowMapSunFactor(float f)
{
    _shadowSunFactor = std::clamp(f, 0.0f, 1.0f);
}

bool EngineVK::DumpShadowMap(const char* /*path*/)
{
    // Not implemented; readback requires staging buffer + host-visible copy.
    return false;
}

bool EngineVK::ShadowMapCacheSelfTest()
{
    return true;
}

// ---------------------------------------------------------------------------
// UpdateShadowFrameConstants
// Fills the shadow UBO fields in _lastFrameConstants before UploadFrameConstants
// copies them to the GPU-visible buffer.
// ---------------------------------------------------------------------------

void EngineVK::UpdateShadowFrameConstants()
{
    const bool active = _shadowTuning.enabled && _shadowMapActive && _shadowSunFactor > 0.01f;

    _lastFrameConstants.shadowCtl[0] = active ? 1.0f : 0.0f;
    _lastFrameConstants.shadowCtl[1] = _shadowTuning.biasBase;
    _lastFrameConstants.shadowCtl[2] =
        1.0f - _shadowSunFactor * (1.0f - _shadowTuning.darkness);
    _lastFrameConstants.shadowCtl[3] =
        (_shadowMapRes > 0) ? (1.0f / static_cast<float>(_shadowMapRes)) : 0.0f;

    if (active)
    {
        const int nC = std::min(_shadowCascades, kShadowCascades);
        for (int c = 0; c < nC; ++c)
            std::memcpy(_lastFrameConstants.cascadeVP[c], _shadowMapVP + c * 16,
                        sizeof(float) * 16);

        for (int c = 0; c < kShadowCascades; ++c)
            _lastFrameConstants.cascadeSplits[c] = (c < nC) ? _shadowSplits[c] : 0.0f;

        _lastFrameConstants.cascadeCtl[0] = static_cast<float>(nC);
        _lastFrameConstants.cascadeCtl[1] = _shadowTuning.fadeRange;
        _lastFrameConstants.cascadeCtl[2] = _shadowTuning.biasBase;
        _lastFrameConstants.cascadeCtl[3] = static_cast<float>(_shadowOmniCount);

        _lastFrameConstants.camFwd[0] = _shadowCamFwd[0];
        _lastFrameConstants.camFwd[1] = _shadowCamFwd[1];
        _lastFrameConstants.camFwd[2] = _shadowCamFwd[2];
        _lastFrameConstants.camFwd[3] = 0.0f;
    }
}

// ---------------------------------------------------------------------------
// EnsureShadowResources
// Idempotent: returns true immediately if res/layers match current allocation.
// Otherwise destroys old resources and re-allocates.
// ---------------------------------------------------------------------------

bool EngineVK::EnsureShadowResources(int res, int layers)
{
    if (!_device || !_physicalDevice)
        return false;

    layers = std::min(layers, kShadowCascades);

    // Already allocated at the right dimensions?
    if (_shadowDepthImage.image != VK_NULL_HANDLE &&
        _shadowMapRes == res && _shadowCascades == layers)
        return true;

    DestroyShadowResources();

    // 1. Depth format
    const VkFormat depthFormat = FindDepthFormat();
    if (depthFormat == VK_FORMAT_UNDEFINED)
    {
        LOG_ERROR(Graphics, "VK shadow: no depth format available");
        return false;
    }

    // 2. Depth image array
    {
        VkImageCreateInfo ii{};
        ii.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ii.imageType     = VK_IMAGE_TYPE_2D;
        ii.format        = depthFormat;
        ii.extent        = {static_cast<uint32_t>(res), static_cast<uint32_t>(res), 1};
        ii.mipLevels     = 1;
        ii.arrayLayers   = static_cast<uint32_t>(layers);
        ii.samples       = VK_SAMPLE_COUNT_1_BIT;
        ii.tiling        = VK_IMAGE_TILING_OPTIMAL;
        ii.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                           VK_IMAGE_USAGE_SAMPLED_BIT;
        ii.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkResult r = vkCreateImage(_device, &ii, nullptr, &_shadowDepthImage.image);
        if (r != VK_SUCCESS)
        {
            LOG_ERROR(Graphics, "VK shadow: vkCreateImage failed: {}", VkResultName(r));
            return false;
        }
        _shadowDepthImage.format    = depthFormat;
        _shadowDepthImage.width     = static_cast<uint32_t>(res);
        _shadowDepthImage.height    = static_cast<uint32_t>(res);
        _shadowDepthImage.mipLevels = 1;

        VkMemoryRequirements req{};
        vkGetImageMemoryRequirements(_device, _shadowDepthImage.image, &req);

        const uint32_t memIdx = vk::FindMemoryType(_physicalDevice, req.memoryTypeBits,
                                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (memIdx == vk::kInvalidMemoryType)
        {
            LOG_ERROR(Graphics, "VK shadow: no device-local memory for depth array");
            vkDestroyImage(_device, _shadowDepthImage.image, nullptr);
            _shadowDepthImage.image = VK_NULL_HANDLE;
            return false;
        }

        VkMemoryAllocateInfo mai{};
        mai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mai.allocationSize  = req.size;
        mai.memoryTypeIndex = memIdx;
        r = vkAllocateMemory(_device, &mai, nullptr, &_shadowDepthImage.memory);
        if (r != VK_SUCCESS)
        {
            LOG_ERROR(Graphics, "VK shadow: vkAllocateMemory failed: {}", VkResultName(r));
            vkDestroyImage(_device, _shadowDepthImage.image, nullptr);
            _shadowDepthImage.image = VK_NULL_HANDLE;
            return false;
        }

        r = vkBindImageMemory(_device, _shadowDepthImage.image, _shadowDepthImage.memory, 0);
        if (r != VK_SUCCESS)
        {
            LOG_ERROR(Graphics, "VK shadow: vkBindImageMemory failed: {}", VkResultName(r));
            DestroyShadowResources();
            return false;
        }

        // Array view -- sampled in the fragment shader as sampler2DArray
        VkImageViewCreateInfo vi{};
        vi.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vi.image                           = _shadowDepthImage.image;
        vi.viewType                        = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        vi.format                          = depthFormat;
        vi.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        vi.subresourceRange.baseMipLevel   = 0;
        vi.subresourceRange.levelCount     = 1;
        vi.subresourceRange.baseArrayLayer = 0;
        vi.subresourceRange.layerCount     = static_cast<uint32_t>(layers);

        r = vkCreateImageView(_device, &vi, nullptr, &_shadowDepthImage.view);
        if (r != VK_SUCCESS)
        {
            LOG_ERROR(Graphics, "VK shadow: vkCreateImageView (array) failed: {}", VkResultName(r));
            DestroyShadowResources();
            return false;
        }
    }

    // 3. Per-layer image views (used as framebuffer attachments)
    for (int i = 0; i < layers; ++i)
    {
        VkImageViewCreateInfo vi{};
        vi.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vi.image                           = _shadowDepthImage.image;
        vi.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        vi.format                          = _shadowDepthImage.format;
        vi.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        vi.subresourceRange.baseMipLevel   = 0;
        vi.subresourceRange.levelCount     = 1;
        vi.subresourceRange.baseArrayLayer = static_cast<uint32_t>(i);
        vi.subresourceRange.layerCount     = 1;

        const VkResult r = vkCreateImageView(_device, &vi, nullptr, &_shadowCascadeViews[i]);
        if (r != VK_SUCCESS)
        {
            LOG_ERROR(Graphics, "VK shadow: vkCreateImageView (layer {}) failed: {}", i,
                      VkResultName(r));
            DestroyShadowResources();
            return false;
        }
    }

    // 4. Comparison sampler. Linear filtering lets the hardware evaluate the
    // four-tap PCF footprint in one filtered depth comparison.
    {
        VkSamplerCreateInfo si{};
        si.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(_physicalDevice, _shadowDepthImage.format, &properties);
        const bool linearFiltering =
            (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0;
        si.magFilter    = linearFiltering ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        si.minFilter    = linearFiltering ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        si.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        si.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.minLod       = 0.0f;
        si.maxLod       = 0.0f;
        si.compareEnable = VK_TRUE;
        si.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        if (!linearFiltering)
            LOG_WARN(Graphics, "VK shadow: depth format lacks linear comparison filtering; using nearest comparison sampling");

        const VkResult r = vkCreateSampler(_device, &si, nullptr, &_shadowSampler);
        if (r != VK_SUCCESS)
        {
            LOG_ERROR(Graphics, "VK shadow: vkCreateSampler failed: {}", VkResultName(r));
            DestroyShadowResources();
            return false;
        }
    }

    // 5. Depth-only render pass
    //    initialLayout = UNDEFINED -> clear on first use
    //    finalLayout   = SHADER_READ_ONLY_OPTIMAL -> ready for sampling
    {
        VkAttachmentDescription depth{};
        depth.format         = _shadowDepthImage.format;
        depth.samples        = VK_SAMPLE_COUNT_1_BIT;
        depth.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        depth.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        depth.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference depthRef{};
        depthRef.attachment = 0;
        depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.pDepthStencilAttachment = &depthRef;

        // Ensure depth writes complete before any fragment shader reads them
        VkSubpassDependency dep{};
        dep.srcSubpass     = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass     = 0;
        dep.srcStageMask   = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dep.dstStageMask   = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.srcAccessMask  = VK_ACCESS_SHADER_READ_BIT;
        dep.dstAccessMask  = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkSubpassDependency dep2{};
        dep2.srcSubpass     = 0;
        dep2.dstSubpass     = VK_SUBPASS_EXTERNAL;
        dep2.srcStageMask   = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dep2.dstStageMask   = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dep2.srcAccessMask  = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dep2.dstAccessMask  = VK_ACCESS_SHADER_READ_BIT;
        dep2.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        const VkSubpassDependency deps[2] = {dep, dep2};

        VkRenderPassCreateInfo rpi{};
        rpi.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpi.attachmentCount = 1;
        rpi.pAttachments    = &depth;
        rpi.subpassCount    = 1;
        rpi.pSubpasses      = &subpass;
        rpi.dependencyCount = 2;
        rpi.pDependencies   = deps;

        const VkResult r = vkCreateRenderPass(_device, &rpi, nullptr, &_shadowRenderPass);
        if (r != VK_SUCCESS)
        {
            LOG_ERROR(Graphics, "VK shadow: vkCreateRenderPass failed: {}", VkResultName(r));
            DestroyShadowResources();
            return false;
        }
    }

    // 6. Per-cascade framebuffers
    for (int i = 0; i < layers; ++i)
    {
        VkFramebufferCreateInfo fbi{};
        fbi.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbi.renderPass      = _shadowRenderPass;
        fbi.attachmentCount = 1;
        fbi.pAttachments    = &_shadowCascadeViews[i];
        fbi.width           = static_cast<uint32_t>(res);
        fbi.height          = static_cast<uint32_t>(res);
        fbi.layers          = 1;

        const VkResult r = vkCreateFramebuffer(_device, &fbi, nullptr, &_shadowFramebuffers[i]);
        if (r != VK_SUCCESS)
        {
            LOG_ERROR(Graphics, "VK shadow: vkCreateFramebuffer (cascade {}) failed: {}", i,
                      VkResultName(r));
            DestroyShadowResources();
            return false;
        }
    }

    // 7. Shadow depth pipelines (solid + alpha)
    if (!CreateGpuShadowResources())
    {
        // GPU CSM is optional. The established direct renderer remains valid
        // when the device has no draw-parameter tier or allocation fails.
        DestroyGpuShadowResources();
    }
    if (!CreateShadowDepthPipeline())
    {
        DestroyShadowResources();
        return false;
    }

    // 8. Update frame descriptor set binding 2 with the shadow sampler+image
    if (_frameDescriptorSet != VK_NULL_HANDLE)
    {
        VkDescriptorImageInfo imgInfo{};
        imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imgInfo.imageView   = _shadowDepthImage.view;
        imgInfo.sampler     = _shadowSampler;

        VkWriteDescriptorSet w = vk::MakeShadowMapDescriptorWrite(_frameDescriptorSet, &imgInfo);
        vkUpdateDescriptorSets(_device, 1, &w, 0, nullptr);
    }

    _shadowMapRes   = res;
    _shadowCascades = layers;
    LOG_INFO(Graphics, "VK shadow: allocated {}x{} depth array ({} cascades, format {})",
             res, res, layers, static_cast<int>(_shadowDepthImage.format));
    return true;
}

// ---------------------------------------------------------------------------
// CreateShadowDepthPipeline
// ---------------------------------------------------------------------------

bool EngineVK::CreateGpuShadowResources()
{
    if (!_gpuSceneCapabilities.GpuDrivenAvailable())
        return true;

    const VkDescriptorSetLayoutBinding bindings[] = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<std::uint32_t>(std::size(bindings));
    layoutInfo.pBindings = bindings;
    if (vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_gpuShadowDescriptorSetLayout) != VK_SUCCESS)
        return false;

    const VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3};
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    if (vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_gpuShadowDescriptorPool) != VK_SUCCESS)
    {
        DestroyGpuShadowResources();
        return false;
    }
    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = _gpuShadowDescriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &_gpuShadowDescriptorSetLayout;
    if (vkAllocateDescriptorSets(_device, &allocateInfo, &_gpuShadowDescriptorSet) != VK_SUCCESS)
    {
        DestroyGpuShadowResources();
        return false;
    }

    VkPushConstantRange cullRange{};
    cullRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    cullRange.size = sizeof(float) * 16 + sizeof(std::uint32_t) * 5;
    VkPipelineLayoutCreateInfo cullLayoutInfo{};
    cullLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    cullLayoutInfo.setLayoutCount = 1;
    cullLayoutInfo.pSetLayouts = &_gpuShadowDescriptorSetLayout;
    cullLayoutInfo.pushConstantRangeCount = 1;
    cullLayoutInfo.pPushConstantRanges = &cullRange;
    if (vkCreatePipelineLayout(_device, &cullLayoutInfo, nullptr, &_gpuShadowCullPipelineLayout) != VK_SUCCESS)
    {
        DestroyGpuShadowResources();
        return false;
    }

    std::vector<std::uint32_t> spirv;
    std::string error;
    if (!CompileShader(kShadowCullComputeSrc, 5, spirv, error))
    {
        LOG_WARN(Graphics, "VK shadow: GPU culler compile failed; using direct caster submission: {}", error);
        DestroyGpuShadowResources();
        return false;
    }
    VkShaderModuleCreateInfo moduleInfo{};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = spirv.size() * sizeof(std::uint32_t);
    moduleInfo.pCode = spirv.data();
    VkShaderModule module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(_device, &moduleInfo, nullptr, &module) != VK_SUCCESS)
    {
        DestroyGpuShadowResources();
        return false;
    }
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = module;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.layout = _gpuShadowCullPipelineLayout;
    const VkResult result = vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                                     &_gpuShadowCullPipeline);
    vkDestroyShaderModule(_device, module, nullptr);
    if (result != VK_SUCCESS || !EnsureGpuShadowCapacity(64, 64))
    {
        DestroyGpuShadowResources();
        return false;
    }
    _gpuShadowEnabled = true;

    const auto makeModule = [&](const char* source, int stage, VkShaderModule& output, const char* name) -> bool
    {
        std::vector<std::uint32_t> shaderSpirv;
        std::string shaderError;
        if (!CompileShader(source, stage, shaderSpirv, shaderError))
        {
            LOG_WARN(Graphics, "VK shadow: {} compile failed: {}", name, shaderError);
            return false;
        }
        VkShaderModuleCreateInfo shaderInfo{};
        shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderInfo.codeSize = shaderSpirv.size() * sizeof(std::uint32_t);
        shaderInfo.pCode = shaderSpirv.data();
        return vkCreateShaderModule(_device, &shaderInfo, nullptr, &output) == VK_SUCCESS;
    };
    constexpr int kVertex = 0;
    constexpr int kFragment = 4;
    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_FRONT_BIT;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;
    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    const VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dyn{};
    dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = static_cast<std::uint32_t>(std::size(dynamicStates));
    dyn.pDynamicStates = dynamicStates;
    VkPipelineViewportStateCreateInfo vp{};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount = 1;

    // GPU shadow pipelines have a distinct set-0 SSBO ABI. This keeps the
    // direct fallback's per-caster push constants intact on lower capability
    // devices while indirect draws fetch transforms by base instance.
    VkPushConstantRange gpuPcRange{};
    gpuPcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    gpuPcRange.size = sizeof(float) * 16;
    VkPipelineLayoutCreateInfo gpuLayoutInfo{};
    gpuLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    gpuLayoutInfo.setLayoutCount = 1;
    gpuLayoutInfo.pSetLayouts = &_gpuShadowDescriptorSetLayout;
    gpuLayoutInfo.pushConstantRangeCount = 1;
    gpuLayoutInfo.pPushConstantRanges = &gpuPcRange;
    if (vkCreatePipelineLayout(_device, &gpuLayoutInfo, nullptr, &_gpuShadowDepthPipelineLayout) != VK_SUCCESS)
        return false;

    const VkDescriptorSetLayout alphaLayouts[] = {_gpuShadowDescriptorSetLayout, _textureDescriptorSetLayout};
    gpuLayoutInfo.setLayoutCount = 2;
    gpuLayoutInfo.pSetLayouts = alphaLayouts;
    if (vkCreatePipelineLayout(_device, &gpuLayoutInfo, nullptr, &_gpuShadowAlphaPipelineLayout) != VK_SUCCESS)
        return false;

    VkShaderModule gpuSolidModule = VK_NULL_HANDLE;
    VkShaderModule gpuAlphaVertexModule = VK_NULL_HANDLE;
    VkShaderModule gpuAlphaFragmentModule = VK_NULL_HANDLE;
    if (!makeModule(kShadowGpuDepthVertSrc, kVertex, gpuSolidModule, "shadowDepthGpu.vert") ||
        !makeModule(kShadowGpuAlphaVertSrc, kVertex, gpuAlphaVertexModule, "shadowDepthGpuAlpha.vert") ||
        !makeModule(kShadowGpuAlphaFragSrc, kFragment, gpuAlphaFragmentModule, "shadowDepthGpuAlpha.frag"))
    {
        if (gpuSolidModule) vkDestroyShaderModule(_device, gpuSolidModule, nullptr);
        if (gpuAlphaVertexModule) vkDestroyShaderModule(_device, gpuAlphaVertexModule, nullptr);
        if (gpuAlphaFragmentModule) vkDestroyShaderModule(_device, gpuAlphaFragmentModule, nullptr);
        return false;
    }

    const VkVertexInputBindingDescription gpuBind = vk::MakeSceneVertexBindingDescription();
    const VkVertexInputAttributeDescription gpuPosition = vk::MakeSceneVertexPositionAttribute();
    VkPipelineVertexInputStateCreateInfo gpuSolidInput{};
    gpuSolidInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    gpuSolidInput.vertexBindingDescriptionCount = 1;
    gpuSolidInput.pVertexBindingDescriptions = &gpuBind;
    gpuSolidInput.vertexAttributeDescriptionCount = 1;
    gpuSolidInput.pVertexAttributeDescriptions = &gpuPosition;
    VkPipelineShaderStageCreateInfo gpuSolidStage{};
    gpuSolidStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    gpuSolidStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    gpuSolidStage.module = gpuSolidModule;
    gpuSolidStage.pName = "main";
    VkGraphicsPipelineCreateInfo gpuPipelineInfo{};
    gpuPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gpuPipelineInfo.stageCount = 1;
    gpuPipelineInfo.pStages = &gpuSolidStage;
    gpuPipelineInfo.pVertexInputState = &gpuSolidInput;
    gpuPipelineInfo.pInputAssemblyState = &ia;
    gpuPipelineInfo.pViewportState = &vp;
    gpuPipelineInfo.pRasterizationState = &rs;
    gpuPipelineInfo.pMultisampleState = &ms;
    gpuPipelineInfo.pDepthStencilState = &ds;
    gpuPipelineInfo.pColorBlendState = &cb;
    gpuPipelineInfo.pDynamicState = &dyn;
    gpuPipelineInfo.layout = _gpuShadowDepthPipelineLayout;
    gpuPipelineInfo.renderPass = _shadowRenderPass;
    if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &gpuPipelineInfo, nullptr,
                                  &_gpuShadowDepthPipeline) != VK_SUCCESS)
    {
        vkDestroyShaderModule(_device, gpuSolidModule, nullptr);
        vkDestroyShaderModule(_device, gpuAlphaVertexModule, nullptr);
        vkDestroyShaderModule(_device, gpuAlphaFragmentModule, nullptr);
        return false;
    }

    const VkVertexInputAttributeDescription gpuAlphaAttributes[] = {
        vk::MakeSceneVertexPositionAttribute(), vk::MakeSceneVertexTexcoordAttribute()};
    VkPipelineVertexInputStateCreateInfo gpuAlphaInput = gpuSolidInput;
    gpuAlphaInput.vertexAttributeDescriptionCount = 2;
    gpuAlphaInput.pVertexAttributeDescriptions = gpuAlphaAttributes;
    VkPipelineRasterizationStateCreateInfo gpuAlphaRaster = rs;
    gpuAlphaRaster.cullMode = VK_CULL_MODE_NONE;
    VkPipelineShaderStageCreateInfo gpuAlphaStages[2]{};
    gpuAlphaStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    gpuAlphaStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    gpuAlphaStages[0].module = gpuAlphaVertexModule;
    gpuAlphaStages[0].pName = "main";
    gpuAlphaStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    gpuAlphaStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    gpuAlphaStages[1].module = gpuAlphaFragmentModule;
    gpuAlphaStages[1].pName = "main";
    gpuPipelineInfo.stageCount = 2;
    gpuPipelineInfo.pStages = gpuAlphaStages;
    gpuPipelineInfo.pVertexInputState = &gpuAlphaInput;
    gpuPipelineInfo.pRasterizationState = &gpuAlphaRaster;
    gpuPipelineInfo.layout = _gpuShadowAlphaPipelineLayout;
    const VkResult gpuAlphaResult = vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &gpuPipelineInfo, nullptr,
                                                               &_gpuShadowAlphaPipeline);
    vkDestroyShaderModule(_device, gpuSolidModule, nullptr);
    vkDestroyShaderModule(_device, gpuAlphaVertexModule, nullptr);
    vkDestroyShaderModule(_device, gpuAlphaFragmentModule, nullptr);
    if (gpuAlphaResult != VK_SUCCESS)
        return false;
    return true;
}

bool EngineVK::EnsureGpuShadowCapacity(std::size_t instanceCount, std::size_t batchCount)
{
    if (!_gpuShadowDescriptorSet || !_device || !_physicalDevice)
        return false;
    const std::size_t requiredInstances = std::max<std::size_t>(instanceCount, 1);
    const std::size_t requiredBatches = std::max<std::size_t>(batchCount, 1);
    if (requiredInstances > _gpuShadowInstanceCapacity || requiredBatches > _gpuShadowBatchCapacity)
    {
        vkDeviceWaitIdle(_device);
        const std::size_t newInstances = std::max(requiredInstances, std::max<std::size_t>(64, _gpuShadowInstanceCapacity * 2));
        const std::size_t newBatches = std::max(requiredBatches, std::max<std::size_t>(64, _gpuShadowBatchCapacity * 2));
        vk::BufferVK instances, indirect, counts;
        if (vk::CreateHostVisibleBuffer(_physicalDevice, _device, newInstances * sizeof(vk::GpuShadowInstanceVK),
                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, instances) != VK_SUCCESS ||
            vk::CreateDeviceLocalBuffer(_physicalDevice, _device,
                                        newInstances * kShadowCascades * sizeof(VkDrawIndexedIndirectCommand),
                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                                            VK_BUFFER_USAGE_TRANSFER_DST_BIT, indirect) != VK_SUCCESS ||
            vk::CreateDeviceLocalBuffer(_physicalDevice, _device, newBatches * kShadowCascades * sizeof(std::uint32_t),
                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                                            VK_BUFFER_USAGE_TRANSFER_DST_BIT, counts) != VK_SUCCESS)
        {
            vk::DestroyBuffer(_device, instances);
            vk::DestroyBuffer(_device, indirect);
            vk::DestroyBuffer(_device, counts);
            return false;
        }
        vk::DestroyBuffer(_device, _gpuShadowInstancesBuffer);
        vk::DestroyBuffer(_device, _gpuShadowIndirectBuffer);
        vk::DestroyBuffer(_device, _gpuShadowCountBuffer);
        _gpuShadowInstancesBuffer = instances;
        _gpuShadowIndirectBuffer = indirect;
        _gpuShadowCountBuffer = counts;
        _gpuShadowInstanceCapacity = newInstances;
        _gpuShadowBatchCapacity = newBatches;
    }
    const VkDescriptorBufferInfo infos[] = {{_gpuShadowInstancesBuffer.buffer, 0, _gpuShadowInstancesBuffer.size},
                                             {_gpuShadowCountBuffer.buffer, 0, _gpuShadowCountBuffer.size},
                                             {_gpuShadowIndirectBuffer.buffer, 0, _gpuShadowIndirectBuffer.size}};
    VkWriteDescriptorSet writes[3]{};
    for (std::uint32_t i = 0; i < std::size(writes); ++i)
    {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = _gpuShadowDescriptorSet;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].pBufferInfo = &infos[i];
    }
    vkUpdateDescriptorSets(_device, static_cast<std::uint32_t>(std::size(writes)), writes, 0, nullptr);
    return true;
}

bool EngineVK::CreateShadowDepthPipeline()
{
    // The shared caster contract supplies one camera-relative world transform
    // per indexed mesh section.  Keep it adjacent to lightVP so depth shaders
    // consume the original scene vertex buffers directly.
    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pcRange.offset     = 0;
    pcRange.size       = sizeof(ShadowPushConstantsVK);

    // --- Solid pipeline layout (no descriptor sets needed) ---
    {
        VkPipelineLayoutCreateInfo li{};
        li.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        li.pushConstantRangeCount = 1;
        li.pPushConstantRanges    = &pcRange;

        const VkResult r = vkCreatePipelineLayout(_device, &li, nullptr,
                                                   &_shadowDepthPipelineLayout);
        if (r != VK_SUCCESS)
        {
            LOG_ERROR(Graphics, "VK shadow: solid pipeline layout failed: {}", VkResultName(r));
            return false;
        }
    }

    // --- Alpha pipeline layout (set 0 = per-texture sampler) ---
    {
        VkPipelineLayoutCreateInfo li{};
        li.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        li.setLayoutCount         = 1;
        li.pSetLayouts            = &_textureDescriptorSetLayout;
        li.pushConstantRangeCount = 1;
        li.pPushConstantRanges    = &pcRange;

        const VkResult r = vkCreatePipelineLayout(_device, &li, nullptr,
                                                   &_shadowAlphaPipelineLayout);
        if (r != VK_SUCCESS)
        {
            LOG_ERROR(Graphics, "VK shadow: alpha pipeline layout failed: {}", VkResultName(r));
            return false;
        }
    }

    // --- Compile shaders (CompileShader takes EShLanguage cast to int) ---
    // EShLangVertex = 0, EShLangFragment = 4
    constexpr int kVertex   = 0;
    constexpr int kFragment = 4;

    auto makeModule = [&](const char* src, int stage,
                          VkShaderModule& mod, const char* name) -> bool
    {
        std::vector<uint32_t> spirv;
        std::string err;
        if (!CompileShader(src, stage, spirv, err))
        {
            LOG_ERROR(Graphics, "VK shadow: {} compile failed: {}", name, err);
            return false;
        }
        VkShaderModuleCreateInfo ci{};
        ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.codeSize = spirv.size() * sizeof(uint32_t);
        ci.pCode    = spirv.data();
        const VkResult r = vkCreateShaderModule(_device, &ci, nullptr, &mod);
        if (r != VK_SUCCESS)
        {
            LOG_ERROR(Graphics, "VK shadow: vkCreateShaderModule ({}) failed: {}", name,
                      VkResultName(r));
            return false;
        }
        return true;
    };

    if (!makeModule(kShadowDepthVertSrc, kVertex,   _shadowDepthVertexModule,  "shadowDepth.vert"))
        return false;
    if (!makeModule(kShadowAlphaVertSrc, kVertex,   _shadowAlphaVertexModule,  "shadowAlpha.vert"))
        return false;
    if (!makeModule(kShadowAlphaFragSrc, kFragment, _shadowAlphaFragmentModule,"shadowAlpha.frag"))
        return false;

    // --- Common fixed-function state ---
    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode    = VK_CULL_MODE_FRONT_BIT; // front-face culling reduces shadow acne
    rs.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE; // engine CW after the viewport Y flip
    rs.lineWidth   = 1.0f;

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable  = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth-only pass -- no colour attachments
    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

    const VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dyn{};
    dyn.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = 2;
    dyn.pDynamicStates    = dynStates;

    VkPipelineViewportStateCreateInfo vp{};
    vp.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount  = 1;

    // --- Solid pipeline: regular scene vertex position ---
    {
        const VkVertexInputBindingDescription bind = vk::MakeSceneVertexBindingDescription();
        const VkVertexInputAttributeDescription attr = vk::MakeSceneVertexPositionAttribute();

        VkPipelineVertexInputStateCreateInfo vi{};
        vi.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.vertexBindingDescriptionCount   = 1;
        vi.pVertexBindingDescriptions      = &bind;
        vi.vertexAttributeDescriptionCount = 1;
        vi.pVertexAttributeDescriptions    = &attr;

        VkPipelineShaderStageCreateInfo stage{};
        stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.stage  = VK_SHADER_STAGE_VERTEX_BIT;
        stage.module = _shadowDepthVertexModule;
        stage.pName  = "main";

        VkGraphicsPipelineCreateInfo gpi{};
        gpi.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        gpi.stageCount          = 1;
        gpi.pStages             = &stage;
        gpi.pVertexInputState   = &vi;
        gpi.pInputAssemblyState = &ia;
        gpi.pViewportState      = &vp;
        gpi.pRasterizationState = &rs;
        gpi.pMultisampleState   = &ms;
        gpi.pDepthStencilState  = &ds;
        gpi.pColorBlendState    = &cb;
        gpi.pDynamicState       = &dyn;
        gpi.layout              = _shadowDepthPipelineLayout;
        gpi.renderPass          = _shadowRenderPass;
        gpi.subpass             = 0;

        const VkResult r = vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &gpi,
                                                      nullptr, &_shadowDepthPipeline);
        if (r != VK_SUCCESS)
        {
            LOG_ERROR(Graphics, "VK shadow: solid pipeline failed: {}", VkResultName(r));
            return false;
        }
    }

    // --- Alpha pipeline: regular scene position + UV ---
    {
        VkPipelineRasterizationStateCreateInfo alphaRs = rs;
        alphaRs.cullMode = VK_CULL_MODE_NONE;
        const VkVertexInputBindingDescription bind = vk::MakeSceneVertexBindingDescription();
        const VkVertexInputAttributeDescription attrs[2] = {
            vk::MakeSceneVertexPositionAttribute(), vk::MakeSceneVertexTexcoordAttribute()};

        VkPipelineVertexInputStateCreateInfo vi{};
        vi.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.vertexBindingDescriptionCount   = 1;
        vi.pVertexBindingDescriptions      = &bind;
        vi.vertexAttributeDescriptionCount = 2;
        vi.pVertexAttributeDescriptions    = attrs;

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = _shadowAlphaVertexModule;
        stages[0].pName  = "main";
        stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = _shadowAlphaFragmentModule;
        stages[1].pName  = "main";

        VkGraphicsPipelineCreateInfo gpi{};
        gpi.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        gpi.stageCount          = 2;
        gpi.pStages             = stages;
        gpi.pVertexInputState   = &vi;
        gpi.pInputAssemblyState = &ia;
        gpi.pViewportState      = &vp;
        gpi.pRasterizationState = &alphaRs;
        gpi.pMultisampleState   = &ms;
        gpi.pDepthStencilState  = &ds;
        gpi.pColorBlendState    = &cb;
        gpi.pDynamicState       = &dyn;
        gpi.layout              = _shadowAlphaPipelineLayout;
        gpi.renderPass          = _shadowRenderPass;
        gpi.subpass             = 0;

        const VkResult r = vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &gpi,
                                                      nullptr, &_shadowAlphaPipeline);
        if (r != VK_SUCCESS)
        {
            LOG_ERROR(Graphics, "VK shadow: alpha pipeline failed: {}", VkResultName(r));
            return false;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// DestroyShadowResources
// ---------------------------------------------------------------------------

bool EngineVK::EnsureShadowRecordingContexts()
{
    for (ShadowRecordingContextVK& context : _shadowRecordingContexts)
    {
        if (context.commandPool && context.commandBuffer)
            continue;

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = _graphicsQueueFamily;
        if (vkCreateCommandPool(_device, &poolInfo, nullptr, &context.commandPool) != VK_SUCCESS)
        {
            DestroyShadowRecordingContexts();
            return false;
        }

        VkCommandBufferAllocateInfo allocation{};
        allocation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocation.commandPool = context.commandPool;
        allocation.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        allocation.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(_device, &allocation, &context.commandBuffer) != VK_SUCCESS)
        {
            DestroyShadowRecordingContexts();
            return false;
        }
    }
    return true;
}

void EngineVK::DestroyShadowRecordingContexts()
{
    for (ShadowRecordingContextVK& context : _shadowRecordingContexts)
    {
        if (context.commandPool)
            vkDestroyCommandPool(_device, context.commandPool, nullptr);
        context = ShadowRecordingContextVK{};
    }
}

void EngineVK::DestroyShadowResources()
{
    if (!_device)
        return;

    vkDeviceWaitIdle(_device);
    DestroyShadowRecordingContexts();
    DestroyGpuShadowResources();

    if (_shadowAlphaPipeline)
    {
        vkDestroyPipeline(_device, _shadowAlphaPipeline, nullptr);
        _shadowAlphaPipeline = VK_NULL_HANDLE;
    }
    if (_shadowAlphaPipelineLayout)
    {
        vkDestroyPipelineLayout(_device, _shadowAlphaPipelineLayout, nullptr);
        _shadowAlphaPipelineLayout = VK_NULL_HANDLE;
    }
    if (_shadowAlphaFragmentModule)
    {
        vkDestroyShaderModule(_device, _shadowAlphaFragmentModule, nullptr);
        _shadowAlphaFragmentModule = VK_NULL_HANDLE;
    }
    if (_shadowAlphaVertexModule)
    {
        vkDestroyShaderModule(_device, _shadowAlphaVertexModule, nullptr);
        _shadowAlphaVertexModule = VK_NULL_HANDLE;
    }
    if (_shadowDepthPipeline)
    {
        vkDestroyPipeline(_device, _shadowDepthPipeline, nullptr);
        _shadowDepthPipeline = VK_NULL_HANDLE;
    }
    if (_shadowDepthPipelineLayout)
    {
        vkDestroyPipelineLayout(_device, _shadowDepthPipelineLayout, nullptr);
        _shadowDepthPipelineLayout = VK_NULL_HANDLE;
    }
    if (_shadowDepthVertexModule)
    {
        vkDestroyShaderModule(_device, _shadowDepthVertexModule, nullptr);
        _shadowDepthVertexModule = VK_NULL_HANDLE;
    }

    for (int i = 0; i < kShadowCascades; ++i)
    {
        if (_shadowFramebuffers[i])
        {
            vkDestroyFramebuffer(_device, _shadowFramebuffers[i], nullptr);
            _shadowFramebuffers[i] = VK_NULL_HANDLE;
        }
        if (_shadowCascadeViews[i])
        {
            vkDestroyImageView(_device, _shadowCascadeViews[i], nullptr);
            _shadowCascadeViews[i] = VK_NULL_HANDLE;
        }
    }

    if (_shadowSampler)
    {
        vkDestroySampler(_device, _shadowSampler, nullptr);
        _shadowSampler = VK_NULL_HANDLE;
    }

    if (_shadowRenderPass)
    {
        vkDestroyRenderPass(_device, _shadowRenderPass, nullptr);
        _shadowRenderPass = VK_NULL_HANDLE;
    }

    vk::DestroyImage(_device, _shadowDepthImage);

    _shadowMapRes    = 0;
    _shadowCascades  = 0;
    _shadowMapActive = false;
}

void EngineVK::DestroyGpuShadowResources()
{
    _gpuShadowEnabled = false;
    _gpuShadowInstances.clear();
    _gpuShadowBatches.clear();
    vk::DestroyBuffer(_device, _gpuShadowInstancesBuffer);
    vk::DestroyBuffer(_device, _gpuShadowIndirectBuffer);
    vk::DestroyBuffer(_device, _gpuShadowCountBuffer);
    _gpuShadowInstanceCapacity = 0;
    _gpuShadowBatchCapacity = 0;
    if (_gpuShadowAlphaPipeline)
        vkDestroyPipeline(_device, _gpuShadowAlphaPipeline, nullptr);
    if (_gpuShadowAlphaPipelineLayout)
        vkDestroyPipelineLayout(_device, _gpuShadowAlphaPipelineLayout, nullptr);
    if (_gpuShadowDepthPipeline)
        vkDestroyPipeline(_device, _gpuShadowDepthPipeline, nullptr);
    if (_gpuShadowDepthPipelineLayout)
        vkDestroyPipelineLayout(_device, _gpuShadowDepthPipelineLayout, nullptr);
    if (_gpuShadowCullPipeline)
        vkDestroyPipeline(_device, _gpuShadowCullPipeline, nullptr);
    if (_gpuShadowCullPipelineLayout)
        vkDestroyPipelineLayout(_device, _gpuShadowCullPipelineLayout, nullptr);
    if (_gpuShadowDescriptorPool)
        vkDestroyDescriptorPool(_device, _gpuShadowDescriptorPool, nullptr);
    if (_gpuShadowDescriptorSetLayout)
        vkDestroyDescriptorSetLayout(_device, _gpuShadowDescriptorSetLayout, nullptr);
    _gpuShadowAlphaPipeline = VK_NULL_HANDLE;
    _gpuShadowAlphaPipelineLayout = VK_NULL_HANDLE;
    _gpuShadowDepthPipeline = VK_NULL_HANDLE;
    _gpuShadowDepthPipelineLayout = VK_NULL_HANDLE;
    _gpuShadowCullPipeline = VK_NULL_HANDLE;
    _gpuShadowCullPipelineLayout = VK_NULL_HANDLE;
    _gpuShadowDescriptorPool = VK_NULL_HANDLE;
    _gpuShadowDescriptorSet = VK_NULL_HANDLE;
    _gpuShadowDescriptorSetLayout = VK_NULL_HANDLE;
}

void EngineVK::RecordGpuShadowCull(VkCommandBuffer commandBuffer, const float* lightVPs, std::uint32_t cascadeCount)
{
    if (!_gpuShadowEnabled || _gpuShadowInstances.empty() || _gpuShadowBatches.empty() || !lightVPs)
        return;
    const VkDeviceSize indirectBytes = static_cast<VkDeviceSize>(_gpuShadowInstanceCapacity) * cascadeCount *
                                       sizeof(VkDrawIndexedIndirectCommand);
    const VkDeviceSize countBytes = static_cast<VkDeviceSize>(_gpuShadowBatchCapacity) * cascadeCount * sizeof(std::uint32_t);
    vkCmdFillBuffer(commandBuffer, _gpuShadowIndirectBuffer.buffer, 0, indirectBytes, 0);
    vkCmdFillBuffer(commandBuffer, _gpuShadowCountBuffer.buffer, 0, countBytes, 0);

    VkBufferMemoryBarrier before[3]{};
    for (VkBufferMemoryBarrier& barrier : before)
    {
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    }
    before[0].srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    before[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    before[0].buffer = _gpuShadowInstancesBuffer.buffer;
    before[0].size = _gpuShadowInstancesBuffer.size;
    before[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    before[1].buffer = _gpuShadowIndirectBuffer.buffer;
    before[1].size = indirectBytes;
    before[2].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    before[2].buffer = _gpuShadowCountBuffer.buffer;
    before[2].size = countBytes;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 3, before, 0, nullptr);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _gpuShadowCullPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _gpuShadowCullPipelineLayout, 0, 1,
                            &_gpuShadowDescriptorSet, 0, nullptr);
    struct ShadowCullConstants
    {
        float lightVP[16];
        std::uint32_t instanceCount;
        std::uint32_t batchCount;
        std::uint32_t cascadeIndex;
        std::uint32_t indirectStride;
        std::uint32_t countStride;
    } constants{};
    static_assert(sizeof(ShadowCullConstants) == sizeof(float) * 16 + sizeof(std::uint32_t) * 5);
    constants.instanceCount = static_cast<std::uint32_t>(_gpuShadowInstances.size());
    constants.batchCount = static_cast<std::uint32_t>(_gpuShadowBatches.size());
    constants.indirectStride = static_cast<std::uint32_t>(_gpuShadowInstanceCapacity);
    constants.countStride = static_cast<std::uint32_t>(_gpuShadowBatchCapacity);
    for (std::uint32_t cascade = 0; cascade < cascadeCount; ++cascade)
    {
        std::memcpy(constants.lightVP, lightVPs + cascade * 16, sizeof(constants.lightVP));
        constants.cascadeIndex = cascade;
        vkCmdPushConstants(commandBuffer, _gpuShadowCullPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                           sizeof(constants), &constants);
        vkCmdDispatch(commandBuffer, (constants.instanceCount + 63u) / 64u, 1, 1);
    }

    VkBufferMemoryBarrier after[2]{};
    for (VkBufferMemoryBarrier& barrier : after)
    {
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    }
    after[0].buffer = _gpuShadowIndirectBuffer.buffer;
    after[0].size = indirectBytes;
    after[1].buffer = _gpuShadowCountBuffer.buffer;
    after[1].size = countBytes;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
                         0, 0, nullptr, 2, after, 0, nullptr);
}

// ---------------------------------------------------------------------------
// RenderShadowDepthScene
// The legacy CPU-flattened caster API is intentionally not consumed by
// Vulkan.  Frame-plan consumers use RenderShadowDepthFramePlan below, which
// binds the source mesh resources directly.  GL retains the historical API.
// ---------------------------------------------------------------------------

void EngineVK::RenderShadowDepthScene(
    const float*           lightVPs,
    const float*           splitViewDist,
    const float*           camFwd3,
    int                    numCascades,
    int                    omniCount,
    int                    res,
    const ShadowCasterSet& casters)
{
    (void)lightVPs;
    (void)splitViewDist;
    (void)camFwd3;
    (void)numCascades;
    (void)omniCount;
    (void)res;
    (void)casters;
    _shadowMapActive = false;
}

void EngineVK::RenderShadowDepthFramePlan(const render::frame::Frame& frame)
{
    const render::frame::ShadowInput& input = frame.shadowInput;
    _shadowSunFactor = input.sunFactor;
    _shadowMapActive = false;
    _shadowResolvedDrawCount = 0;
    _shadowCascadeDrawCounts.fill(0);
    if (!_shadowTuning.enabled || !input.enabled || input.sunFactor <= 0.01f || !_device || !_commandPool ||
        !_graphicsQueue)
        return;

    const auto preparationStarted = std::chrono::steady_clock::now();
    const std::vector<vk::ShadowDrawCommandVK> commands = vk::BuildShadowDrawCommands(input);
    if (commands.empty())
        return;

    struct ResolvedShadowDraw
    {
        const vk::MeshResourcesVK* mesh = nullptr;
        VkDescriptorSet alphaDescriptor = VK_NULL_HANDLE;
        ShadowPushConstantsVK constants;
        std::uint32_t firstIndex = 0;
        std::uint32_t indexCount = 0;
        std::uint32_t meshId = 0;
        std::uint32_t alphaTextureId = 0;
        bool alphaTested = false;
        GfxMatrix world = {};
    };

    std::vector<ResolvedShadowDraw> resolvedDraws;
    resolvedDraws.reserve(commands.size());
    for (const vk::ShadowDrawCommandVK& command : commands)
    {
        const render::frame::ShadowCaster& caster = input.casters[command.casterIndex];
        const vk::MeshResourcesVK* mesh = _meshRegistry.Resolve(command.meshId);
        if (!mesh || !mesh->IsValid() || command.firstIndex >= mesh->indexCount)
            continue;

        const std::uint32_t indexCount = std::min(command.indexCount, mesh->indexCount - command.firstIndex);
        if (indexCount == 0)
            continue;

        ResolvedShadowDraw draw;
        draw.mesh = mesh;
        draw.firstIndex = command.firstIndex;
        draw.indexCount = indexCount;
        draw.meshId = command.meshId;
        draw.alphaTested = command.alphaMode == render::frame::ShadowCasterAlphaMode::Cutout &&
                           _shadowAlphaPipeline != VK_NULL_HANDLE;
        draw.alphaTextureId = draw.alphaTested ? caster.alphaTexture.id : 0;
        draw.world = caster.world;
        if (draw.alphaTested)
        {
            TextureVK* texture = ResolveTexture(draw.alphaTextureId);
            // The main frame command buffer records newly decoded textures
            // after this shadow submission is prepared. Do not sample an
            // image still in UNDEFINED layout here; use an opaque depth draw
            // for this one shadow update, then alpha-test it next frame.
            if (!texture || !texture->IsGpuReadyForSampling())
                draw.alphaTested = false;
            draw.alphaDescriptor = draw.alphaTested ? texture->GetDescriptorSet() : VK_NULL_HANDLE;
            if (draw.alphaDescriptor == VK_NULL_HANDLE && _fallbackWhiteTexture)
                draw.alphaDescriptor = _fallbackWhiteTexture->GetDescriptorSet();
        }
        std::memcpy(draw.constants.worldColumns, &caster.world, sizeof(draw.constants.worldColumns));
        draw.constants.translation[0] = caster.world._41;
        draw.constants.translation[1] = caster.world._42;
        draw.constants.translation[2] = caster.world._43;
        draw.constants.alphaCutoff = caster.alphaCutoff;
        resolvedDraws.push_back(draw);
    }
    if (resolvedDraws.empty())
        return;

    // Depth-only shadow draws have no blending dependency. Grouping identical
    // resource state eliminates redundant binds in each cascade.
    std::sort(resolvedDraws.begin(), resolvedDraws.end(), [](const ResolvedShadowDraw& lhs,
                                                              const ResolvedShadowDraw& rhs)
    {
        if (lhs.alphaTested != rhs.alphaTested)
            return lhs.alphaTested < rhs.alphaTested;
        if (lhs.meshId != rhs.meshId)
            return lhs.meshId < rhs.meshId;
        if (lhs.alphaTextureId != rhs.alphaTextureId)
            return lhs.alphaTextureId < rhs.alphaTextureId;
        return lhs.firstIndex < rhs.firstIndex;
    });

    namespace sm = Poseidon::shadow;
    sm::CascadeBuildParams bp;
    bp.camPos = {frame.cameraPosition[0], frame.cameraPosition[1], frame.cameraPosition[2]};
    bp.forward = {input.camera.forward[0], input.camera.forward[1], input.camera.forward[2]};
    bp.right = {input.camera.right[0], input.camera.right[1], input.camera.right[2]};
    bp.up = {input.camera.up[0], input.camera.up[1], input.camera.up[2]};
    bp.tanHalfX = input.camera.tanHalfX;
    bp.tanHalfY = input.camera.tanHalfY;
    bp.nearD = input.camera.nearDistance;
    bp.farD = input.camera.farDistance;
    bp.sunDir = {frame.sunDirection[0], frame.sunDirection[1], frame.sunDirection[2]};
    bp.count = _shadowTuning.cascadeCount;
    bp.distanceCoef = _shadowTuning.distanceCoef;
    bp.splitCoef = _shadowTuning.splitCoef;
    bp.resolution = _shadowTuning.resolution;
    bp.zPad = 50.0f;
    bp.omniCount = _shadowTuning.omniCount;
    bp.omniCoef[0] = _shadowTuning.omniCoef0;
    bp.omniCoef[1] = _shadowTuning.omniCoef1;
    const sm::CascadeSet cascades = sm::BuildShadowCascadesTiered(bp);
    if (cascades.count <= 0 || !EnsureShadowResources(_shadowTuning.resolution, cascades.count))
        return;
    if (!EnsureShadowRecordingContexts())
        return;
    _cpuShadowPrepareMs =
        std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - preparationStarted).count();

    const bool gpuShadow = _gpuShadowEnabled && _gpuShadowCullPipeline && _gpuShadowDepthPipeline &&
                           _gpuShadowAlphaPipeline;
    std::array<std::vector<const ResolvedShadowDraw*>, 4> visibleDraws;
    _shadowResolvedDrawCount = static_cast<std::uint32_t>(resolvedDraws.size());
    for (int cascadeIndex = 0; cascadeIndex < cascades.count; ++cascadeIndex)
    {
        std::vector<const ResolvedShadowDraw*>& visible = visibleDraws[cascadeIndex];
        visible.reserve(resolvedDraws.size());
        for (const ResolvedShadowDraw& draw : resolvedDraws)
            visible.push_back(&draw);
        _shadowCascadeDrawCounts[cascadeIndex] = static_cast<std::uint32_t>(visible.size());
    }

    // The previous shadow submission only needs to complete before its command
    // pools are reset, allowing all serial setup above to overlap it.
    if (_shadowInFlight)
        vkWaitForFences(_device, 1, &_shadowInFlight, VK_TRUE, UINT64_MAX);

    if (gpuShadow)
    {
        _gpuShadowInstances.clear();
        _gpuShadowBatches.clear();
        _gpuShadowInstances.reserve(resolvedDraws.size());
        _gpuShadowBatches.reserve(resolvedDraws.size());
        for (const ResolvedShadowDraw& draw : resolvedDraws)
        {
            const bool append = !_gpuShadowBatches.empty() &&
                                _gpuShadowBatches.back().meshId == draw.meshId &&
                                _gpuShadowBatches.back().alphaTextureId == draw.alphaTextureId &&
                                _gpuShadowBatches.back().alphaTested == (draw.alphaTested ? 1u : 0u);
            if (!append)
            {
                vk::GpuShadowBatchVK batch;
                batch.firstInstance = static_cast<std::uint32_t>(_gpuShadowInstances.size());
                batch.indirectOffset = batch.firstInstance;
                batch.countOffset = static_cast<std::uint32_t>(_gpuShadowBatches.size());
                batch.meshId = draw.meshId;
                batch.alphaTextureId = draw.alphaTextureId;
                batch.alphaTested = draw.alphaTested ? 1u : 0u;
                _gpuShadowBatches.push_back(batch);
            }
            vk::GpuShadowBatchVK& batch = _gpuShadowBatches.back();
            vk::GpuShadowInstanceVK instance;
            instance.localBoundsCenter[0] = draw.mesh->localBoundsCenter[0];
            instance.localBoundsCenter[1] = draw.mesh->localBoundsCenter[1];
            instance.localBoundsCenter[2] = draw.mesh->localBoundsCenter[2];
            instance.localBoundsCenter[3] = draw.mesh->localBoundsRadius;
            std::memcpy(instance.world, &draw.world, sizeof(instance.world));
            instance.batchIndex = batch.countOffset;
            instance.indirectOffset = batch.indirectOffset;
            instance.firstIndex = draw.firstIndex;
            instance.indexCount = draw.indexCount;
            instance.alphaCutoff = draw.constants.alphaCutoff;
            ++batch.instanceCount;
            _gpuShadowInstances.push_back(instance);
        }
        for (const vk::GpuShadowBatchVK& batch : _gpuShadowBatches)
        {
            const std::uint32_t end = batch.firstInstance + batch.instanceCount;
            for (std::uint32_t i = batch.firstInstance; i < end; ++i)
                _gpuShadowInstances[i].batchCapacity = batch.instanceCount;
        }
        if (!EnsureGpuShadowCapacity(_gpuShadowInstances.size(), _gpuShadowBatches.size()))
        {
            LOG_WARN(Graphics, "VK shadow: GPU buffer growth failed; retaining direct caster submission");
            _gpuShadowEnabled = false;
            _gpuShadowInstances.clear();
            _gpuShadowBatches.clear();
        }
        else
            vk::UploadMappedBuffer(_gpuShadowInstancesBuffer, _gpuShadowInstances.data(),
                                   _gpuShadowInstances.size() * sizeof(vk::GpuShadowInstanceVK));
    }

    const bool recordGpuShadow = gpuShadow && _gpuShadowEnabled && !_gpuShadowInstances.empty() &&
                                 !_gpuShadowBatches.empty();

    const int res = _shadowTuning.resolution;
    const auto recordCascades = [&](std::uint32_t begin, std::uint32_t end)
    {
        for (std::uint32_t cascadeIndex = begin; cascadeIndex < end; ++cascadeIndex)
        {
            ShadowRecordingContextVK& context = _shadowRecordingContexts[cascadeIndex];
            context.recorded = false;
            if (vkResetCommandPool(_device, context.commandPool, 0) != VK_SUCCESS)
                continue;

            VkCommandBufferInheritanceInfo inheritance{};
            inheritance.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
            inheritance.renderPass = _shadowRenderPass;
            inheritance.subpass = 0;
            inheritance.framebuffer = _shadowFramebuffers[cascadeIndex];

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT |
                              VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
            beginInfo.pInheritanceInfo = &inheritance;
            if (vkBeginCommandBuffer(context.commandBuffer, &beginInfo) != VK_SUCCESS)
                continue;

            VkViewport viewport{};
            viewport.y = static_cast<float>(res);
            viewport.width = static_cast<float>(res);
            viewport.height = -static_cast<float>(res);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(context.commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.extent = {static_cast<std::uint32_t>(res), static_cast<std::uint32_t>(res)};
            vkCmdSetScissor(context.commandBuffer, 0, 1, &scissor);

            if (recordGpuShadow)
            {
                float lightVP[16] = {};
                std::memcpy(lightVP, cascades.camRelVP[cascadeIndex].m.data(), sizeof(lightVP));
                VkPipeline lastPipeline = VK_NULL_HANDLE;
                VkBuffer lastVertexBuffer = VK_NULL_HANDLE;
                VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
                VkDescriptorSet lastAlphaDescriptor = VK_NULL_HANDLE;
                for (const vk::GpuShadowBatchVK& batch : _gpuShadowBatches)
                {
                    if (batch.instanceCount == 0)
                        continue;
                    const bool alpha = batch.alphaTested != 0;
                    const VkPipeline pipeline = alpha ? _gpuShadowAlphaPipeline : _gpuShadowDepthPipeline;
                    const VkPipelineLayout layout = alpha ? _gpuShadowAlphaPipelineLayout : _gpuShadowDepthPipelineLayout;
                    if (pipeline != lastPipeline)
                    {
                        vkCmdBindPipeline(context.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                        vkCmdBindDescriptorSets(context.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1,
                                                &_gpuShadowDescriptorSet, 0, nullptr);
                        vkCmdPushConstants(context.commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                                           sizeof(lightVP), lightVP);
                        lastPipeline = pipeline;
                        lastAlphaDescriptor = VK_NULL_HANDLE;
                    }
                    if (alpha)
                    {
                        TextureVK* texture = ResolveTexture(batch.alphaTextureId);
                        VkDescriptorSet alphaDescriptor = texture ? texture->GetDescriptorSet() : VK_NULL_HANDLE;
                        if (!alphaDescriptor && _fallbackWhiteTexture)
                            alphaDescriptor = _fallbackWhiteTexture->GetDescriptorSet();
                        if (alphaDescriptor && alphaDescriptor != lastAlphaDescriptor)
                        {
                            vkCmdBindDescriptorSets(context.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1,
                                                    &alphaDescriptor, 0, nullptr);
                            lastAlphaDescriptor = alphaDescriptor;
                        }
                    }
                    const vk::MeshResourcesVK* mesh = _meshRegistry.Resolve(batch.meshId);
                    if (!mesh || !mesh->IsValid())
                        continue;
                    const VkDeviceSize vertexOffset = 0;
                    if (mesh->vertexBuffer != lastVertexBuffer)
                    {
                        vkCmdBindVertexBuffers(context.commandBuffer, 0, 1, &mesh->vertexBuffer, &vertexOffset);
                        lastVertexBuffer = mesh->vertexBuffer;
                    }
                    if (mesh->indexBuffer != lastIndexBuffer)
                    {
                        vkCmdBindIndexBuffer(context.commandBuffer, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT16);
                        lastIndexBuffer = mesh->indexBuffer;
                    }
                    const VkDeviceSize indirectOffset =
                        (static_cast<VkDeviceSize>(cascadeIndex) * _gpuShadowInstanceCapacity + batch.indirectOffset) *
                        sizeof(VkDrawIndexedIndirectCommand);
                    const VkDeviceSize countOffset =
                        (static_cast<VkDeviceSize>(cascadeIndex) * _gpuShadowBatchCapacity + batch.countOffset) *
                        sizeof(std::uint32_t);
                    if (_gpuSceneCapabilities.drawIndirectCount)
                    {
                        vkCmdDrawIndexedIndirectCount(context.commandBuffer, _gpuShadowIndirectBuffer.buffer, indirectOffset,
                                                      _gpuShadowCountBuffer.buffer, countOffset, batch.instanceCount,
                                                      sizeof(VkDrawIndexedIndirectCommand));
                    }
                    else
                    {
                        vkCmdDrawIndexedIndirect(context.commandBuffer, _gpuShadowIndirectBuffer.buffer, indirectOffset,
                                                 batch.instanceCount, sizeof(VkDrawIndexedIndirectCommand));
                    }
                }
                context.recorded = vkEndCommandBuffer(context.commandBuffer) == VK_SUCCESS;
                continue;
            }

            VkPipeline lastPipeline = VK_NULL_HANDLE;
            VkDescriptorSet lastAlphaDescriptor = VK_NULL_HANDLE;
            VkBuffer lastVertexBuffer = VK_NULL_HANDLE;
            VkBuffer lastIndexBuffer = VK_NULL_HANDLE;
            for (const ResolvedShadowDraw* drawPtr : visibleDraws[cascadeIndex])
            {
                const ResolvedShadowDraw& draw = *drawPtr;
                const VkDeviceSize vertexOffset = 0;
                if (draw.mesh->vertexBuffer != lastVertexBuffer)
                {
                    vkCmdBindVertexBuffers(context.commandBuffer, 0, 1, &draw.mesh->vertexBuffer, &vertexOffset);
                    lastVertexBuffer = draw.mesh->vertexBuffer;
                }
                if (draw.mesh->indexBuffer != lastIndexBuffer)
                {
                    vkCmdBindIndexBuffer(context.commandBuffer, draw.mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT16);
                    lastIndexBuffer = draw.mesh->indexBuffer;
                }

                const VkPipeline pipeline = draw.alphaTested ? _shadowAlphaPipeline : _shadowDepthPipeline;
                const VkPipelineLayout pipelineLayout =
                    draw.alphaTested ? _shadowAlphaPipelineLayout : _shadowDepthPipelineLayout;
                if (pipeline == VK_NULL_HANDLE)
                    continue;
                if (pipeline != lastPipeline)
                {
                    vkCmdBindPipeline(context.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                    lastPipeline = pipeline;
                    if (draw.alphaTested)
                        lastAlphaDescriptor = VK_NULL_HANDLE;
                }
                if (draw.alphaTested && draw.alphaDescriptor != VK_NULL_HANDLE &&
                    draw.alphaDescriptor != lastAlphaDescriptor)
                {
                    vkCmdBindDescriptorSets(context.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0,
                                            1, &draw.alphaDescriptor, 0, nullptr);
                    lastAlphaDescriptor = draw.alphaDescriptor;
                }

                ShadowPushConstantsVK constants = draw.constants;
                std::memcpy(constants.lightVP, cascades.camRelVP[cascadeIndex].m.data(), sizeof(constants.lightVP));
                vkCmdPushConstants(context.commandBuffer, pipelineLayout,
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants),
                                   &constants);
                vkCmdDrawIndexed(context.commandBuffer, draw.indexCount, 1, draw.firstIndex, 0, 0);
            }

            context.recorded = vkEndCommandBuffer(context.commandBuffer) == VK_SUCCESS;
        }
    };
    const auto secondaryRecordStarted = std::chrono::steady_clock::now();
    if (TaskPool* taskPool = GetGlobalTaskPool(); taskPool && cascades.count > 1)
        taskPool->ParallelFor(static_cast<std::uint32_t>(cascades.count), recordCascades);
    else
        recordCascades(0, static_cast<std::uint32_t>(cascades.count));
    _cpuShadowSecondaryRecordMs =
        std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - secondaryRecordStarted).count();
    for (int c = 0; c < cascades.count; ++c)
    {
        if (!_shadowRecordingContexts[c].recorded)
        {
            LOG_WARN(Graphics, "VK shadow: cascade {} secondary command recording failed", c);
            return;
        }
    }

    // InitDraw has waited for the prior frame fence, so this buffer is no
    // longer in use. Submit it before the main frame on the same queue rather
    // than draining the queue with vkQueueWaitIdle every frame.
    if (_shadowCommandBuffer == VK_NULL_HANDLE)
    {
        VkCommandBufferAllocateInfo ai{};
        ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool        = _commandPool;
        ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(_device, &ai, &_shadowCommandBuffer) != VK_SUCCESS)
            return;
    }
    VkCommandBuffer cb = _shadowCommandBuffer;
    vkResetCommandBuffer(cb, 0);

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &bi);
    if (_shadowTimingQueryPool)
    {
        vkCmdResetQueryPool(cb, _shadowTimingQueryPool, 0, 2);
        vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, _shadowTimingQueryPool, 0);
    }

    if (recordGpuShadow)
        RecordGpuShadowCull(cb, cascades.camRelVP[0].m.data(), static_cast<std::uint32_t>(cascades.count));

    // --- One render pass per cascade, with worker-recorded secondary commands ---
    for (int c = 0; c < cascades.count; ++c)
    {
        VkClearValue clearVal{};
        clearVal.depthStencil.depth   = 1.0f;
        clearVal.depthStencil.stencil = 0;

        VkRenderPassBeginInfo rpi{};
        rpi.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpi.renderPass        = _shadowRenderPass;
        rpi.framebuffer       = _shadowFramebuffers[c];
        rpi.renderArea.offset = {0, 0};
        rpi.renderArea.extent = {static_cast<uint32_t>(res), static_cast<uint32_t>(res)};
        rpi.clearValueCount   = 1;
        rpi.pClearValues      = &clearVal;

        vkCmdBeginRenderPass(cb, &rpi, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        const VkCommandBuffer secondary = _shadowRecordingContexts[c].commandBuffer;
        vkCmdExecuteCommands(cb, 1, &secondary);
        vkCmdEndRenderPass(cb);
    }

    if (_shadowTimingQueryPool)
        vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, _shadowTimingQueryPool, 1);

    if (vkEndCommandBuffer(cb) != VK_SUCCESS)
        return;

    // Same-queue submission order makes the final shadow layout readable by
    // the main scene command buffer without a CPU-side queue stall.
    VkSubmitInfo si{};
    si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers    = &cb;
    if (_shadowInFlight)
        vkResetFences(_device, 1, &_shadowInFlight);
    const VkResult submitResult = vkQueueSubmit(_graphicsQueue, 1, &si, _shadowInFlight);
    if (submitResult != VK_SUCCESS)
    {
        LOG_ERROR(Graphics, "VK shadow: queue submit failed: {}", VkResultName(submitResult));
        if (_shadowInFlight)
            RecreateSignaledShadowFence(_device, _shadowInFlight);
        return;
    }
    _shadowTimingPending = _shadowTimingQueryPool != VK_NULL_HANDLE;

    // Persist cascade data for UpdateShadowFrameConstants
    _shadowCascades  = cascades.count;
    _shadowOmniCount = cascades.omniCount;
    _shadowCamFwd[0] = input.camera.forward[0];
    _shadowCamFwd[1] = input.camera.forward[1];
    _shadowCamFwd[2] = input.camera.forward[2];
    for (int c = 0; c < cascades.count; ++c)
    {
        std::memcpy(_shadowMapVP + c * 16, cascades.camRelVP[c].m.data(), sizeof(float) * 16);
        _shadowSplits[c] = cascades.splitViewDist[c];
    }
    _shadowMapActive = true;
}

} // namespace Poseidon
