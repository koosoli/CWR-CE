#pragma once

#include <PoseidonVK/BufferVK.hpp>
#include <Poseidon/Graphics/Rendering/Frame/Frame.hpp>
#include <Poseidon/World/Terrain/TerrainCdlod.hpp>

#include <cstdint>
#include <vector>

namespace Poseidon::vk
{
// Vulkan ownership for the immutable terrain map contract.  Raster activation
// is intentionally separate: no caller may select this path until its terrain
// self-shadow and sky-visibility resources are populated too.
class TerrainVK
{
  public:
    static constexpr std::uint32_t kGridN = 32;
    // This is a policy ceiling, not a shader limit.  Initialize clamps it to
    // the selected device's ordinary sampled-image descriptor limits.
    static constexpr std::uint32_t kRequestedLayerCapacity = 256;
    struct GridVertex { float x, z, skirt; };
    struct NodeInstance { float originX, originZ, size; std::uint32_t lod; float morphStart, morphEnd; };
    struct Params
    {
        float worldOrigin[2] = {};
        float landGrid = 1.0f, terrainGrid = 1.0f;
        std::uint32_t heightWidth = 1, heightHeight = 1, landRange = 1, layerCount = 0;
        float seaLevel = 0.0f, time = 0.0f, swashSpeed = 0.15f, swashAmplitude = 0.15f;
        float wetHeight = 1.2f, wetDarken = 0.55f, _pad[2] = {};
    };
    static_assert(sizeof(GridVertex) == 12);
    static_assert(sizeof(NodeInstance) == 24);
    // std140 layout consumed by terrain.vert/frag: vec2 + 2 floats, uvec4,
    // vec4 water, vec4 wetness.
    static_assert(sizeof(Params) == 64);
    enum DescriptorBinding : std::uint32_t
    {
        kHeightmapBinding = 0,
        kIndexMapBinding = 1,
        kJitterMapBinding = 2,
        kParamsBinding = 3,
        kTextureLayersBinding = 4,
    };
    struct LayerBinding
    {
        VkDescriptorImageInfo image = {};
        // A missing/dead TextureHandle is invalid. A registered texture whose
        // upload has not completed is valid, but uses the deterministic
        // fallback for this descriptor update.
        bool sourceValid = true;
        bool usesFallback = false;
    };
    struct DescriptorTelemetry
    {
        std::uint32_t capacity = 0;
        std::uint32_t requestedLayers = 0;
        std::uint32_t boundLayers = 0;
        std::uint32_t fallbackLayers = 0;
        std::uint32_t invalidLayers = 0;
        std::uint32_t invalidLayerIndices = 0;
    };
    // Set 2 is deliberately external.  It cannot be represented by a white
    // fallback: the terrain receiver must have its own self-shadow, detail,
    // blend and sky-visibility resources before it can replace legacy terrain.
    struct RasterInputs
    {
        VkDescriptorSetLayout frameDescriptorSetLayout = VK_NULL_HANDLE; // set 0: FrameConstants + CSM
        VkDescriptorSetLayout visualDescriptorSetLayout = VK_NULL_HANDLE; // set 2: terrain visual inputs
        VkDescriptorSet frameDescriptorSet = VK_NULL_HANDLE;
        VkDescriptorSet visualDescriptorSet = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkExtent2D extent = {};
        bool csmBound = false;
        bool selfShadowBound = false;
        bool detailBound = false;
        bool blendBound = false;
        bool skyVisibilityBound = false;

        bool Complete() const noexcept
        {
            return frameDescriptorSetLayout != VK_NULL_HANDLE && visualDescriptorSetLayout != VK_NULL_HANDLE &&
                   frameDescriptorSet != VK_NULL_HANDLE && visualDescriptorSet != VK_NULL_HANDLE &&
                   renderPass != VK_NULL_HANDLE && extent.width != 0 && extent.height != 0 && csmBound &&
                   selfShadowBound && detailBound && blendBound && skyVisibilityBound;
        }
    };
    // Required bindings in RasterInputs::visualDescriptorSetLayout (set 2).
    // Keep these synchronized with terrain.frag.glsl when the producer pass is
    // added; no descriptor layout is created here because no real producer
    // images exist yet.
    enum VisualInputBinding : std::uint32_t
    {
        kSelfShadowBinding = 0,
        kDetailBinding = 1,
        kBlendBinding = 2,
        kSkyVisibilityBinding = 3,
    };

    bool Initialize(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue queue);
    void Destroy(VkDevice device);
    bool Upload(const render::frame::TerrainOpaque& terrain);
    // Called by EngineVK after TextureHandle IDs have been resolved to native
    // image/sampler pairs. No UPDATE_AFTER_BIND flag is needed: dedicated
    // terrain rasterization is not installed, and future raster activation
    // must update this set only after its prior use has completed.
    bool UpdateLayerDescriptors(const std::vector<LayerBinding>& layers);
    // Compiles the CMake-embedded GLSL and creates a pipeline compatible with
    // set 0 (frame/CSM), this object's set 1, and the future visual-input set 2.
    // Returns false until all non-optional visual receiver resources exist.
    bool CreateRasterPipeline(const RasterInputs& inputs);
    void DestroyRasterPipeline(VkDevice device);
    // Records only geometry bindings and one indexed instanced draw.  It does
    // not bind the pipeline or descriptors, so calling it cannot activate the
    // dedicated terrain path by accident.
    bool RecordInstancedGridDraw(VkCommandBuffer commandBuffer) const;
    void Select(float cameraX, float cameraY, float cameraZ, float visibleX0, float visibleZ0, float visibleX1,
                float visibleZ1);
    bool Ready() const noexcept { return _ready; }
    std::uint64_t Revision() const noexcept { return _revision.revision; }
    const std::vector<NodeInstance>& VisibleNodes() const noexcept { return _visible; }
    const BufferVK& GridVertices() const noexcept { return _gridVertices; }
    const BufferVK& GridIndices() const noexcept { return _gridIndices; }
    const BufferVK& Instances() const noexcept { return _instances; }
    const ImageVK& Heightmap() const noexcept { return _heightmap; }
    const ImageVK& IndexMap() const noexcept { return _indexMap; }
    const ImageVK& JitterMap() const noexcept { return _jitterMap; }
    // Kept separate from Parameters(): descriptor setup must bind the actual
    // GPU allocation, not a transient CPU Params snapshot.
    const BufferVK& ParamsBuffer() const noexcept { return _paramsBuffer; }
    const Params& Parameters() const noexcept { return _params; }
    VkDescriptorSetLayout DescriptorSetLayout() const noexcept { return _descriptorSetLayout; }
    VkDescriptorSet DescriptorSet() const noexcept { return _descriptorSet; }
    VkPipelineLayout RasterPipelineLayout() const noexcept { return _rasterPipelineLayout; }
    VkPipeline RasterPipeline() const noexcept { return _rasterPipeline; }
    bool RasterPipelineReady() const noexcept { return _rasterPipeline != VK_NULL_HANDLE; }
    std::uint32_t LayerCapacity() const noexcept { return _layerCapacity; }
    const DescriptorTelemetry& Telemetry() const noexcept { return _telemetry; }

  private:
    bool CreateGrid();
    bool RecreateMapImages(const render::frame::TerrainOpaque& terrain);
    bool CreateDescriptorResources();
    bool CreateMapSamplers();
    bool UpdateStaticDescriptors();
    bool ValidateLayerIndices(const render::frame::TerrainOpaque& terrain);
    void DestroyDescriptorResources(VkDevice device);
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    VkCommandPool _commandPool = VK_NULL_HANDLE;
    VkQueue _queue = VK_NULL_HANDLE;
    BufferVK _gridVertices, _gridIndices, _instances, _paramsBuffer;
    ImageVK _heightmap, _indexMap, _jitterMap;
    VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet _descriptorSet = VK_NULL_HANDLE;
    VkSampler _heightSampler = VK_NULL_HANDLE;
    VkSampler _indexSampler = VK_NULL_HANDLE;
    VkSampler _jitterSampler = VK_NULL_HANDLE;
    VkPipelineLayout _rasterPipelineLayout = VK_NULL_HANDLE;
    VkPipeline _rasterPipeline = VK_NULL_HANDLE;
    std::uint32_t _gridIndexCount = 0;
    std::uint32_t _layerCapacity = 0;
    DescriptorTelemetry _telemetry;
    CdlodRevisionCache _revision;
    std::vector<CdlodNode> _tree;
    std::vector<float> _ranges;
    std::vector<NodeInstance> _visible;
    int _root = -1, _levels = 0;
    float _leafSize = 0.0f;
    Params _params;
    bool _ready = false;
};
} // namespace Poseidon::vk
