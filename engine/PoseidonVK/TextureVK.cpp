// TextureVK.cpp — Vulkan texture implementation.
//
// Format mapping from Poseidon's PacFormat to VkFormat, then a staging-buffer
// path: create host-visible buffer, decode mip via ITextureSource, copy to
// device-local VkImage with layout transitions.

#include <PoseidonVK/TextureVK.hpp>
#include <PoseidonVK/EngineVK.hpp>

#include <Poseidon/Graphics/Core/MipmapLayout.hpp>
#include <Poseidon/Graphics/Rendering/Font/Pactext.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Graphics/Textures/LooseTextures.hpp>
#include <Poseidon/Graphics/Textures/PAADecoder.hpp>
#include <Poseidon/IO/FileServer.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Core/Config/EngineConfig.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>

#include <algorithm>
#include <bit>
#include <cstring>
#include <vector>

namespace Poseidon
{

// ---------------------------------------------------------------------------
// Format mapping
// ---------------------------------------------------------------------------
namespace
{
struct FormatInfo
{
    VkFormat vkFmt;
    bool compressed;
};

// DstFormatVK: maps a texture's source (file) format to the in-memory
// decode target that TextureVK will upload to Vulkan.
// Mirrors TextureGL33_Init::DstFormat() but without GL-specific hardware
// capability checks. P8 palette-expand requires a conversion.
//
// NOTE: PacARGB4444 and PacARGB1555 stay at their native format here so that
// GetMipmapData / LoadPaaBin16's '_sFormat == _dFormat' assertion passes.
// A transcoding step in UploadMips() then converts the raw 16-bit pixels to
// 32-bit RGBA8 (VK_FORMAT_R8G8B8A8_UNORM) before uploading, matching what
// GL33 achieves via GL_BGRA + GL_UNSIGNED_SHORT_*_REV.
static PacFormat DstFormatVK(PacFormat srcFormat)
{
    switch (srcFormat)
    {
        case PacP8:       return PacARGB1555; // palette-expand: 8-bit indexed → 16-bit ARGB1555
        default:          return srcFormat;   // identity: keep native format for GetMipmapData
    }
}

FormatInfo PacFormatToVk(PacFormat fmt)
{
    switch (fmt)
    {
        case PacDXT1: return {VK_FORMAT_BC1_RGBA_UNORM_BLOCK, true};
        case PacDXT2: // fallthrough
        case PacDXT3: return {VK_FORMAT_BC2_UNORM_BLOCK, true};
        case PacDXT4: // fallthrough
        case PacDXT5: return {VK_FORMAT_BC3_UNORM_BLOCK, true};
        // PacARGB8888: decoded bytes come out as [R][G][B][A] from the in-memory
        // transcoder (argb4444/1555 → rgba8) and are uploaded as RGBA8_UNORM.
        // PacARGB4444 / PacARGB1555 also map here because UploadMips transcodes
        // those to RGBA8 before upload.
        case PacARGB8888:  return {VK_FORMAT_R8G8B8A8_UNORM, false};
        case PacARGB4444:  return {VK_FORMAT_R8G8B8A8_UNORM, false}; // transcoded in UploadMips
        case PacARGB1555:  return {VK_FORMAT_R8G8B8A8_UNORM, false}; // transcoded in UploadMips
        case PacRGB565:    return {VK_FORMAT_R5G6B5_UNORM_PACK16, false};
        case PacAI88:      return {VK_FORMAT_R8G8B8A8_UNORM, false}; // transcoded in UploadMips: [grey,alpha]->[grey,grey,grey,alpha]
        default:           return {VK_FORMAT_R8G8B8A8_UNORM, false};
    }
}

static std::uint32_t s_nextTextureId = TextureVK::kFallbackResourceId + 1;
} // namespace

std::uint32_t TextureVK::AllocateResourceId() noexcept
{
    return s_nextTextureId++;
}

// ---------------------------------------------------------------------------
// Ctor / dtor
// ---------------------------------------------------------------------------
TextureVK::TextureVK(EngineVK& engine)
    : _engine(engine)
{
}

TextureVK::~TextureVK()
{
    _engine.UnregisterTexture(this);
    _engine.CancelTextureUpload(this);
    // The texture can be released while the prior frame still samples it, or
    // after this frame has recorded its upload.  EngineVK owns the retirement
    // queue and destroys these only after _inFlight has completed.
    std::vector<vk::BufferVK> pendingStaging;
    pendingStaging.reserve(_pendingMipUploads.size());
    for (PendingMipUpload& upload : _pendingMipUploads)
        pendingStaging.push_back(upload.staging);
    _pendingMipUploads.clear();
    _engine.RetireTextureResources(std::move(_image), _sampler, _descriptorSet,
                                   _samplerVariants, _descriptorVariants, std::move(pendingStaging));
    _image = {};
    _sampler = VK_NULL_HANDLE;
    _descriptorSet = VK_NULL_HANDLE;
    _samplerVariants = {};
    _descriptorVariants = {};
}

// ---------------------------------------------------------------------------
// Init — load source, allocate GPU image, upload every mip
// ---------------------------------------------------------------------------
bool TextureVK::Init(RStringB name)
{
    SetName(name);
    _resourceId = AllocateResourceId();

    // Resolve loose/mod texture overrides exactly as GL33 does (TextureGL33_Init.cpp:171).
    // Without this, modded textures and any path redirects registered with the loose
    // texture system are silently ignored, falling through to the fallback black texture.
    RString resolved = Poseidon::Graphics::ResolveLooseTexturePath(name);

    ITextureSourceFactory* factory = SelectTextureSourceFactory(resolved);
    if (!factory || !factory->Check(resolved))
    {
        LOG_DEBUG(Graphics, "TextureVK: no source for '{}', using fallback ID", (const char*)name);
        // Leave _image null — sampler will also be null.  The frame extractor
        // still gets a unique resource ID so draws are tracked correctly.
        return true; // not a hard failure: game continues with missing-texture colour
    }

    // Match the established GL33 data path: parse the complete source on the
    // render thread while the VFS is available. Every valid load therefore has
    // dimensions, image, descriptor, and registry entry before frame capture.
    _src = factory->Create(resolved, _mipmaps, MAX_MIPMAPS);
    if (!_src)
        return true;

    const PacFormat sourceFormat = _src->GetFormat();
    if (sourceFormat == PacARGB4444 || sourceFormat == PacAI88 || sourceFormat == PacARGB8888)
        _src->ForceAlpha();

    // Count usable mip levels (stop at 2×2 minimum — mirrors TextureDummy)
    const int totalMips = std::min(_src->GetMipmapCount(), MAX_MIPMAPS);
    _nMipmaps = 0;
    for (int i = 0; i < totalMips; ++i)
    {
        if (_mipmaps[i]._w < 2 || _mipmaps[i]._h < 2)
            break;
        ++_nMipmaps;
    }
    if (_nMipmaps == 0)
        return true;

    _w = _mipmaps[0]._w;
    _h = _mipmaps[0]._h;

    if (!CmpStartStr(Name(), "fonts\\"))
        _maxSize = 1024;
    else if (!CmpStartStr(Name(), "merged\\"))
        _maxSize = 2048;
    else if (AbstractTextBank::AnimatedNumber(Name()) >= 0 && IsAlpha())
        _maxSize = ENGINE_CONFIG.maxAnimText;
    else
        _maxSize = ENGINE_CONFIG.maxObjText;

    _nativeSrcFormat = sourceFormat;
    const PacFormat uploadFmt = DstFormatVK(_nativeSrcFormat);
    const FormatInfo fi = PacFormatToVk(uploadFmt);
    const bool canGenerateMips = sourceFormat != PacDXT1 && sourceFormat != PacDXT2 &&
                                 sourceFormat != PacDXT3 && sourceFormat != PacDXT4 &&
                                 sourceFormat != PacDXT5;
    const std::uint32_t levels = canGenerateMips
                                     ? std::bit_width(std::max(static_cast<std::uint32_t>(_w),
                                                                static_cast<std::uint32_t>(_h)))
                                     : static_cast<std::uint32_t>(_nMipmaps);
    for (int i = 0; i < _nMipmaps; ++i)
        _mipmaps[i].SetDestFormat(uploadFmt, 8);
    if (vk::CreateImage2D(_engine._physicalDevice, _engine._device, static_cast<std::uint32_t>(_w),
                          static_cast<std::uint32_t>(_h), levels, fi.vkFmt,
                          VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _image) != VK_SUCCESS)
    {
        LOG_ERROR(Graphics, "TextureVK: CreateImage2D failed for '{}'", (const char*)name);
        return true;
    }
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = _engine._maxSamplerAnisotropy;
    samplerInfo.maxLod = static_cast<float>(levels - 1);
    if (vkCreateSampler(_engine._device, &samplerInfo, nullptr, &_sampler) != VK_SUCCESS)
    {
        vk::DestroyImage(_engine._device, _image);
        LOG_WARN(Graphics, "TextureVK: sampler creation failed for '{}'", (const char*)name);
        return true;
    }
    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = _engine._textureDescriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &_engine._textureDescriptorSetLayout;
    if (vkAllocateDescriptorSets(_engine._device, &allocateInfo, &_descriptorSet) != VK_SUCCESS)
    {
        vkDestroySampler(_engine._device, _sampler, nullptr); _sampler = VK_NULL_HANDLE;
        vk::DestroyImage(_engine._device, _image);
        LOG_WARN(Graphics, "TextureVK: descriptor allocation failed for '{}'", (const char*)name);
        return true;
    }
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = _image.view;
    imageInfo.sampler = _sampler;
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _descriptorSet;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(_engine._device, 1, &write, 0, nullptr);

    // The descriptor is registered synchronously with texture creation. The
    // frame command records its staged copies before the scene pass, allowing
    // all normal textures loaded during setup to bind on that first frame.
    _engine.RegisterTexture(this);
    if (!UploadMips())
        LOG_WARN(Graphics, "TextureVK: no mip data queued for '{}'", (const char*)name);
    return true;
}

bool TextureVK::UploadMips()
{
    if (!_src || !_image.image || _nMipmaps <= 0)
        return false;
    for (PendingMipUpload& upload : _pendingMipUploads)
        _engine.RetireTextureStaging(upload.staging);
    _pendingMipUploads.clear();

    const PacFormat sharedFmt = _mipmaps[0].DstFormat();
    const bool transcode4444 = sharedFmt == PacARGB4444;
    const bool transcode1555 = sharedFmt == PacARGB1555;
    const bool transcode8888 = sharedFmt == PacARGB8888;
    const bool transcodeAI88 = sharedFmt == PacAI88;
    const bool needsTranscode = transcode4444 || transcode1555 || transcode8888 || transcodeAI88;
    _fileMipCount = static_cast<std::uint32_t>(_nMipmaps);

    for (std::uint32_t level = 0; level < _fileMipCount; ++level)
    {
        const PacLevelMem& srcMip = _mipmaps[level];
        PacLevelMem mip = srcMip;
        if (mip.DstFormat() != sharedFmt)
            mip.SetDestFormat(sharedFmt, 8);
        const auto layout = render::mipmap::ComputeLayout(sharedFmt, srcMip._w, srcMip._h);
        const std::size_t dataSize = static_cast<std::size_t>(layout.dataSize);
        if (dataSize == 0)
            continue;
        std::vector<std::uint8_t> sourcePixels(dataSize);
        if (!_src->GetMipmapData(sourcePixels.data(), mip, static_cast<int>(level)))
            std::memset(sourcePixels.data(), 0, sourcePixels.size());

        const std::size_t pixelCount = static_cast<std::size_t>(srcMip._w) * srcMip._h;
        std::vector<std::uint8_t> transcodedPixels;
        const void* uploadPixels = sourcePixels.data();
        std::size_t uploadSize = sourcePixels.size();
        if (needsTranscode)
        {
            transcodedPixels.resize(pixelCount * 4);
            auto* dst = transcodedPixels.data();
            if (transcode4444 || transcode1555)
            {
                const auto* src = reinterpret_cast<const std::uint16_t*>(sourcePixels.data());
                for (std::size_t pixel = 0; pixel < pixelCount; ++pixel)
                {
                    const std::uint16_t value = src[pixel];
                    if (transcode4444)
                    {
                        dst[pixel * 4] = ((value >> 8) & 15) * 17;
                        dst[pixel * 4 + 1] = ((value >> 4) & 15) * 17;
                        dst[pixel * 4 + 2] = (value & 15) * 17;
                        dst[pixel * 4 + 3] = ((value >> 12) & 15) * 17;
                    }
                    else
                    {
                        dst[pixel * 4] = ((value >> 10) & 31) * 255 / 31;
                        dst[pixel * 4 + 1] = ((value >> 5) & 31) * 255 / 31;
                        dst[pixel * 4 + 2] = (value & 31) * 255 / 31;
                        dst[pixel * 4 + 3] = (value & 0x8000) ? 255 : 0;
                    }
                }
            }
            else if (transcodeAI88)
            {
                for (std::size_t pixel = 0; pixel < pixelCount; ++pixel)
                {
                    dst[pixel * 4] = sourcePixels[pixel * 2];
                    dst[pixel * 4 + 1] = sourcePixels[pixel * 2];
                    dst[pixel * 4 + 2] = sourcePixels[pixel * 2];
                    dst[pixel * 4 + 3] = sourcePixels[pixel * 2 + 1];
                }
            }
            else
            {
                for (std::size_t pixel = 0; pixel < pixelCount; ++pixel)
                {
                    dst[pixel * 4] = sourcePixels[pixel * 4 + 2];
                    dst[pixel * 4 + 1] = sourcePixels[pixel * 4 + 1];
                    dst[pixel * 4 + 2] = sourcePixels[pixel * 4];
                    dst[pixel * 4 + 3] = sourcePixels[pixel * 4 + 3];
                }
            }
            uploadPixels = transcodedPixels.data();
            uploadSize = transcodedPixels.size();
        }

        PendingMipUpload upload;
        if (vk::CreateHostVisibleBuffer(_engine._physicalDevice, _engine._device,
                                        static_cast<VkDeviceSize>(uploadSize),
                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, upload.staging) != VK_SUCCESS)
            continue;
        vk::UploadMappedBuffer(upload.staging, uploadPixels, uploadSize);
        upload.mipLevel = level;
        upload.width = static_cast<std::uint32_t>(srcMip._w);
        upload.height = static_cast<std::uint32_t>(srcMip._h);
        _pendingMipUploads.push_back(upload);
    }

    if (_pendingMipUploads.empty())
        return false;
    const bool canGenerateMips = _nativeSrcFormat != PacDXT1 && _nativeSrcFormat != PacDXT2 &&
                                 _nativeSrcFormat != PacDXT3 && _nativeSrcFormat != PacDXT4 &&
                                 _nativeSrcFormat != PacDXT5;
    if (canGenerateMips)
        _nMipmaps = static_cast<int>(std::bit_width(std::max(static_cast<std::uint32_t>(_w), static_cast<std::uint32_t>(_h))));
    _initialUploadPending = true;
    _gpuReadyForSampling = false;
    _engine.QueueTextureUpload(this);
    return true;
}

bool TextureVK::QueueDynamicUpload(const void* rgba, std::uint32_t size)
{
    if (!_image.image || !rgba || size == 0)
        return false;

    // A dynamic texture can be updated repeatedly before the frame command
    // buffer is built. Only the latest CPU contents need copying. Retire the
    // replaced staging buffer through the same fence-owned path so texture
    // resource destruction has one lifetime rule.
    for (PendingMipUpload& upload : _pendingMipUploads)
        _engine.RetireTextureStaging(upload.staging);
    _pendingMipUploads.clear();

    PendingMipUpload upload;
    if (vk::CreateHostVisibleBuffer(_engine._physicalDevice, _engine._device, size,
                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, upload.staging) != VK_SUCCESS)
        return false;
    vk::UploadMappedBuffer(upload.staging, rgba, size);
    upload.width = static_cast<std::uint32_t>(_w);
    upload.height = static_cast<std::uint32_t>(_h);
    _pendingMipUploads.push_back(upload);
    _initialUploadPending = !_imageLayoutInitialized;
    _fileMipCount = 1;
    _gpuReadyForSampling = false;
    _engine.QueueTextureUpload(this);
    return true;
}

bool TextureVK::RecordPendingUpload(VkCommandBuffer commandBuffer)
{
    if (!commandBuffer || !_image.image || _pendingMipUploads.empty())
        return false;

    const auto imageBarrier = [&](VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                                  VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                                  std::uint32_t baseMip, std::uint32_t mipCount,
                                  VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = _image.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = baseMip;
        barrier.subresourceRange.levelCount = mipCount;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = srcAccess;
        barrier.dstAccessMask = dstAccess;
        vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    };

    const std::uint32_t fullMipCount = _image.mipLevels;
    const std::uint32_t fileMipCount = std::min(_fileMipCount, fullMipCount);
    const bool generateMips = _initialUploadPending && fullMipCount > fileMipCount;
    const VkImageLayout priorLayout = _initialUploadPending ? VK_IMAGE_LAYOUT_UNDEFINED
                                                              : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    const VkPipelineStageFlags priorStage = _initialUploadPending ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                                                                    : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    const VkAccessFlags priorAccess = _initialUploadPending ? 0 : VK_ACCESS_SHADER_READ_BIT;

    imageBarrier(priorStage, VK_PIPELINE_STAGE_TRANSFER_BIT, priorAccess, VK_ACCESS_TRANSFER_WRITE_BIT,
                 0, _initialUploadPending ? fileMipCount : 1,
                 priorLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    for (const PendingMipUpload& upload : _pendingMipUploads)
    {
        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = upload.mipLevel;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {upload.width, upload.height, 1};
        vkCmdCopyBufferToImage(commandBuffer, upload.staging.buffer, _image.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    if (generateMips)
    {
        imageBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                     VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                     0, fileMipCount, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        int srcW = _w;
        int srcH = _h;
        for (std::uint32_t level = 1; level < fileMipCount; ++level)
        {
            srcW = std::max(srcW / 2, 1);
            srcH = std::max(srcH / 2, 1);
        }
        for (std::uint32_t level = fileMipCount; level < fullMipCount; ++level)
        {
            const int dstW = std::max(srcW / 2, 1);
            const int dstH = std::max(srcH / 2, 1);
            imageBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, VK_ACCESS_TRANSFER_WRITE_BIT, level, 1,
                         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            VkImageBlit blit{};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = level - 1;
            blit.srcSubresource.layerCount = 1;
            blit.srcOffsets[1] = {srcW, srcH, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = level;
            blit.dstSubresource.layerCount = 1;
            blit.dstOffsets[1] = {dstW, dstH, 1};
            vkCmdBlitImage(commandBuffer, _image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           _image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
            if (level + 1 < fullMipCount)
            {
                imageBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, level, 1,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            }
            srcW = dstW;
            srcH = dstH;
        }
        imageBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                     VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, 0, fullMipCount - 1,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        imageBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                     VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, fullMipCount - 1, 1,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    else
    {
        imageBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                     VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                     0, _initialUploadPending ? fileMipCount : 1,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    // This command buffer records transfers before every scene draw, so its
    // descriptor is safe to bind later in this same command buffer. Ownership
    // and final layout state still remain pending until queue submission.
    _uploadRecorded = true;
    return true;
}

void TextureVK::CommitPendingUpload(std::vector<vk::BufferVK>& inFlightStaging)
{
    for (PendingMipUpload& upload : _pendingMipUploads)
        inFlightStaging.push_back(upload.staging);
    _pendingMipUploads.clear();
    _initialUploadPending = false;
    _uploadRecorded = false;
    _imageLayoutInitialized = true;
    // The submitted transfer precedes every main-scene draw on this graphics
    // queue; later same-queue shadow submissions can sample it as well.
    _gpuReadyForSampling = true;
}

void TextureVK::DiscardPendingUpload() noexcept
{
    _uploadRecorded = false;
}

VkDescriptorSet TextureVK::GetDescriptorSet() const
{
    if (_descriptorSet && (_gpuReadyForSampling || _uploadRecorded || _resourceId == kFallbackResourceId))
        return _descriptorSet;
    if (_engine._fallbackWhiteTexture)
        return _engine._fallbackWhiteTexture->_descriptorSet;
    return VK_NULL_HANDLE;
}

VkDescriptorSet TextureVK::GetDescriptorSet(std::uint32_t samplerFilter, std::uint32_t samplerClamp) const
{
    const std::uint32_t key = ((samplerFilter & 1u) << 2) | (samplerClamp & 3u);
    if (!_gpuReadyForSampling && !_uploadRecorded && _resourceId != kFallbackResourceId)
        return GetDescriptorSet();
    if (!_image.view || !_engine._device || !_engine._textureDescriptorPool)
        return GetDescriptorSet();
    if (key == _baseSamplerKey)
        return GetDescriptorSet();
    if (_descriptorVariants[key])
        return _descriptorVariants[key];

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = samplerFilter == 1u ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    samplerInfo.minFilter = samplerInfo.magFilter;
    samplerInfo.mipmapMode = samplerFilter == 1u ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = (samplerClamp & 1u) ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = (samplerClamp & 2u) ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = samplerInfo.addressModeV;
    samplerInfo.maxLod = static_cast<float>(std::max(_nMipmaps - 1, 0));
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = _engine._maxSamplerAnisotropy;

    VkSampler sampler = VK_NULL_HANDLE;
    if (vkCreateSampler(_engine._device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
        return GetDescriptorSet();

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = _engine._textureDescriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &_engine._textureDescriptorSetLayout;

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(_engine._device, &allocateInfo, &descriptorSet) != VK_SUCCESS)
    {
        vkDestroySampler(_engine._device, sampler, nullptr);
        return GetDescriptorSet();
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = _image.view;
    imageInfo.sampler = sampler;
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSet;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(_engine._device, 1, &write, 0, nullptr);

    _samplerVariants[key] = sampler;
    _descriptorVariants[key] = descriptorSet;
    return descriptorSet;
}

// ---------------------------------------------------------------------------
// Texture virtuals
// ---------------------------------------------------------------------------
bool TextureVK::IsTransparent() const { return _src && _src->IsTransparent(); }
bool TextureVK::IsAlpha() const { return _src && _src->IsAlpha(); }
Color TextureVK::GetColor() { return _src ? _src->GetAverageColor() : HBlack; }

Color TextureVK::GetPixel(int level, float u, float v) const
{
    if (!_src || level < 0 || level >= _nMipmaps)
        return HWhite;

    const PacLevelMem& mip = _mipmaps[level];
    const std::size_t dataSize = static_cast<std::size_t>(mip._pitch) * mip._h;
    if (dataSize == 0)
        return HWhite;

    std::vector<char> pixels(dataSize);
    if (!_src->GetMipmapData(pixels.data(), mip, level))
        return HWhite;
    return mip.GetPixel(pixels.data(), u, v);
}

AlphaStats::Kind TextureVK::GetAlphaClass()
{
    if (_alphaClass >= 0)
        return static_cast<AlphaStats::Kind>(_alphaClass);

    AlphaStats::Kind kind = AlphaStats::Opaque;
    if (_src)
    {
        const bool hasAlpha = _src->IsAlpha();
        const bool chroma = _src->IsTransparent();
        const bool oneBit = _src->GetFormat() == PacDXT1;
        AlphaStats decoded;
        const AlphaStats* decodedPtr = nullptr;

        if (hasAlpha && !oneBit)
        {
            QIFStream input;
            GFileServer->Open(input, Name());
            const int size = input.fail() ? 0 : input.rest();
            if (size > 0)
            {
                std::vector<std::uint8_t> sourceBytes(static_cast<std::size_t>(size));
                input.read(reinterpret_cast<char*>(sourceBytes.data()), size);
                const char* name = Name();
                const std::size_t len = name ? std::strlen(name) : 0;
                const bool isPaa = len >= 4 && (name[len - 1] == 'a' || name[len - 1] == 'A');
                const DecodedImage image = DecodePAABuffer(sourceBytes.data(), sourceBytes.size(), isPaa);
                if (image.valid())
                {
                    decoded = ClassifyAlpha(image.rgba.data(), image.rgba.size() / 4);
                    decodedPtr = &decoded;
                }
            }
        }
        kind = ClassifyTextureAlpha(hasAlpha, chroma, oneBit, decodedPtr);
    }

    _alphaClass = static_cast<signed char>(kind);
    return kind;
}

} // namespace Poseidon

