#pragma once

#include <PoseidonVK/BufferVK.hpp>
#include <Poseidon/Graphics/Textures/TextureBank.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace Poseidon
{
class EngineVK;

// Vulkan-owned texture. Mirrors the TextureDummy/TextureGL33 pattern:
// Init() decodes the source via the engine-neutral ITextureSource factory,
// creates a device-local VkImage via a host-visible staging buffer, and
// registers a process-local resource ID so the frame extractor can track it.
class TextureVK : public Texture
{
    friend class TextBankVK;
    friend class EngineVK;

public:
    explicit TextureVK(EngineVK& engine);
    ~TextureVK() override;

    bool Init(RStringB name);

    // Opaque ID used by the frame extractor (parallel to TextureGL33::GetResourceId).
    std::uint32_t GetResourceId() const noexcept { return _resourceId; }
    static std::uint32_t AllocateResourceId() noexcept;

    // --- Texture virtuals ---
    int AWidth(int /*level*/ = 0) const override { return _w; }
    int AHeight(int /*level*/ = 0) const override { return _h; }
    int ANMipmaps() const override { return _nMipmaps; }
    void ASetNMipmaps(int n) override
    {
        if (n > _nMipmaps)
            n = _nMipmaps;
        if (n < 1)
            n = 1;
        _nMipmaps = n;
    }

    Color GetPixel(int level, float u, float v) const override;
    Color GetColor() override;
    bool IsTransparent() const override;
    bool IsAlpha() const override;
    AlphaStats::Kind GetAlphaClass() override;

    void SetMaxSize(int sz) override { _maxSize = sz; }
    int AMaxSize() const override { return _maxSize; }

    const PacLevelMem& AMipmap(int level) const override { return _mipmaps[level]; }
    PacLevelMem& AMipmap(int level) override { return _mipmaps[level]; }
    bool VerifyChecksum(const MipInfo&) const override { return true; }

    static constexpr std::uint32_t kFallbackResourceId = 1;

    VkDescriptorSet GetDescriptorSet() const;
    VkDescriptorSet GetDescriptorSet(std::uint32_t samplerFilter, std::uint32_t samplerClamp) const;
    bool HasValidGpuImage() const noexcept override
    {
        return _image.image != VK_NULL_HANDLE && _image.view != VK_NULL_HANDLE &&
               _sampler != VK_NULL_HANDLE && _descriptorSet != VK_NULL_HANDLE;
    }
    // The descriptor becomes usable once its copy/layout transition has been
    // recorded ahead of the scene pass on the frame command buffer.
    bool IsGpuReadyForSampling() const noexcept { return _gpuReadyForSampling; }

    // Returns the native image/sampler pair without exposing ownership.  This
    // is intentionally independent of IsGpuReadyForSampling(): descriptor
    // writers use the latter to decide whether a source is eligible, while a
    // known fallback may be queued for upload in the current frame and still
    // needs a valid descriptor payload.
    bool GetSampledImageInfo(VkDescriptorImageInfo& out) const noexcept;

private:
    struct PendingMipUpload
    {
        vk::BufferVK staging;
        std::uint32_t mipLevel = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
    };

    bool UploadMips();
    bool QueueDynamicUpload(const void* rgba, std::uint32_t size);
    bool RecordPendingUpload(VkCommandBuffer commandBuffer);
    void CommitPendingUpload(std::vector<vk::BufferVK>& inFlightStaging);
    void DiscardPendingUpload() noexcept;

    EngineVK& _engine;
    std::uint32_t _resourceId = 0;
    SRef<ITextureSource> _src;

    int _w = 0;
    int _h = 0;
    int _nMipmaps = 0;
    int _maxSize = 256;
    PacLevelMem _mipmaps[MAX_MIPMAPS];
    // Stores the raw source format before DstFormatVK mapping.
    // Used by UploadMips to select the 16\u219232 transcoder for formats that
    // cannot be natively uploaded to Vulkan with correct channel order.
    PacFormat _nativeSrcFormat = PacARGB1555;

    vk::ImageVK _image;
    VkSampler _sampler = VK_NULL_HANDLE;
    VkDescriptorSet _descriptorSet = VK_NULL_HANDLE;
    mutable std::array<VkSampler, 8> _samplerVariants = {};
    mutable std::array<VkDescriptorSet, 8> _descriptorVariants = {};
    std::uint32_t _baseSamplerKey = 0;
    // This becomes true only after the command buffer containing the initial
    // UNDEFINED -> transfer -> shader-read transition was successfully
    // submitted.  A queued/recorded upload is not yet an initialized layout.
    bool _imageLayoutInitialized = false;
    bool _initialUploadPending = false;
    bool _uploadRecorded = false;
    bool _gpuReadyForSampling = false;
    std::uint32_t _fileMipCount = 0;
    std::vector<PendingMipUpload> _pendingMipUploads;
    signed char _alphaClass = -1;
};

} // namespace Poseidon
