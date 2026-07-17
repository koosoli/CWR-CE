#include <PoseidonVK/TerrainVK.hpp>
#include <PoseidonVK/RenderStateVK.hpp>
#include <PoseidonVK/ShaderCompilerVK.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace
{
constexpr const char kTerrainVertexShader[] =
#include <PoseidonVK/Shaders/terrain.vert.glsl.hpp>
;
constexpr const char kTerrainFragmentShader[] =
#include <PoseidonVK/Shaders/terrain.frag.glsl.hpp>
;
} // namespace

namespace Poseidon::vk
{
bool TerrainVK::Initialize(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue queue)
{
    _physicalDevice = physicalDevice;
    _device = device;
    _commandPool = commandPool;
    _queue = queue;
    if (!device || !physicalDevice || !commandPool || !queue)
        return false;
    if (!CreateDescriptorResources() || !CreateMapSamplers() || !CreateGrid())
    {
        Destroy(device);
        return false;
    }
    if (CreateHostVisibleBuffer(physicalDevice, device, 8192 * sizeof(NodeInstance), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                _instances) != VK_SUCCESS ||
        CreateHostVisibleBuffer(physicalDevice, device, sizeof(Params), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, _paramsBuffer) !=
            VK_SUCCESS)
    {
        Destroy(device);
        return false;
    }
    UploadMappedBuffer(_paramsBuffer, &_params, sizeof(_params));
    _ready = true;
    return true;
}

bool TerrainVK::CreateDescriptorResources()
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(_physicalDevice, &properties);
    // There are three fixed combined samplers (height, index, jitter). The
    // variable binding is sized against the normal descriptor limits because
    // the set is never changed while bound; UPDATE_AFTER_BIND is deliberately
    // neither enabled nor consumed here.
    const std::uint32_t maxCombined = std::min({properties.limits.maxDescriptorSetSampledImages,
                                                properties.limits.maxPerStageDescriptorSampledImages,
                                                properties.limits.maxDescriptorSetSamplers,
                                                properties.limits.maxPerStageDescriptorSamplers});
    if (maxCombined <= 3)
        return false;
    _layerCapacity = std::min(kRequestedLayerCapacity, maxCombined - 3);

    std::array<VkDescriptorSetLayoutBinding, 5> bindings{};
    for (std::uint32_t binding = kHeightmapBinding; binding <= kJitterMapBinding; ++binding)
    {
        bindings[binding].binding = binding;
        bindings[binding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[binding].descriptorCount = 1;
        bindings[binding].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    bindings[kParamsBinding].binding = kParamsBinding;
    bindings[kParamsBinding].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[kParamsBinding].descriptorCount = 1;
    bindings[kParamsBinding].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    // Vulkan requires a variable-count binding to be the highest-numbered
    // binding in its layout. Binding 4 is therefore reserved for native
    // TextureVK layer images.
    bindings[kTextureLayersBinding].binding = kTextureLayersBinding;
    bindings[kTextureLayersBinding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[kTextureLayersBinding].descriptorCount = _layerCapacity;
    bindings[kTextureLayersBinding].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorBindingFlags, 5> bindingFlags{};
    bindingFlags[kTextureLayersBinding] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
                                          VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
    flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    flagsInfo.bindingCount = static_cast<std::uint32_t>(bindingFlags.size());
    flagsInfo.pBindingFlags = bindingFlags.data();

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = &flagsInfo;
    layoutInfo.bindingCount = static_cast<std::uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    if (vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS)
        return false;

    const std::array<VkDescriptorPoolSize, 2> poolSizes = {{
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _layerCapacity + 3},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
    }};
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = static_cast<std::uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    if (vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS)
        return false;

    VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountInfo{};
    variableCountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    variableCountInfo.descriptorSetCount = 1;
    variableCountInfo.pDescriptorCounts = &_layerCapacity;
    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.pNext = &variableCountInfo;
    allocateInfo.descriptorPool = _descriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &_descriptorSetLayout;
    return vkAllocateDescriptorSets(_device, &allocateInfo, &_descriptorSet) == VK_SUCCESS;
}

bool TerrainVK::CreateMapSamplers()
{
    const auto createSampler = [&](VkFilter filter, VkSampler& sampler)
    {
        VkSamplerCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter = filter;
        info.minFilter = filter;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.maxLod = 0.0f;
        return vkCreateSampler(_device, &info, nullptr, &sampler) == VK_SUCCESS;
    };
    return createSampler(VK_FILTER_LINEAR, _heightSampler) && createSampler(VK_FILTER_NEAREST, _indexSampler) &&
           createSampler(VK_FILTER_NEAREST, _jitterSampler);
}

bool TerrainVK::UpdateStaticDescriptors()
{
    if (!_descriptorSet || !_heightmap.view || !_indexMap.view || !_jitterMap.view || !_paramsBuffer.buffer ||
        !_heightSampler || !_indexSampler || !_jitterSampler)
        return false;

    const std::array<VkDescriptorImageInfo, 3> images = {{
        {_heightSampler, _heightmap.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {_indexSampler, _indexMap.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {_jitterSampler, _jitterMap.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
    }};
    VkDescriptorBufferInfo params{};
    params.buffer = _paramsBuffer.buffer;
    params.offset = 0;
    params.range = sizeof(Params);
    std::array<VkWriteDescriptorSet, 4> writes{};
    for (std::uint32_t binding = 0; binding < images.size(); ++binding)
    {
        writes[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[binding].dstSet = _descriptorSet;
        writes[binding].dstBinding = binding;
        writes[binding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[binding].descriptorCount = 1;
        writes[binding].pImageInfo = &images[binding];
    }
    writes[kParamsBinding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[kParamsBinding].dstSet = _descriptorSet;
    writes[kParamsBinding].dstBinding = kParamsBinding;
    writes[kParamsBinding].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[kParamsBinding].descriptorCount = 1;
    writes[kParamsBinding].pBufferInfo = &params;
    vkUpdateDescriptorSets(_device, static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);
    return true;
}

bool TerrainVK::ValidateLayerIndices(const render::frame::TerrainOpaque& terrain)
{
    _telemetry.capacity = _layerCapacity;
    _telemetry.requestedLayers = static_cast<std::uint32_t>(terrain.textureLayers.size());
    _telemetry.invalidLayerIndices = 0;
    const std::size_t layerCount = terrain.textureLayers.size();
    // A variable descriptor count is fixed at set allocation. Even an unused
    // excess layer would make the captured layer array unbindable, so reject
    // it before uploading any map that could later index the wrong set.
    if (layerCount == 0 || layerCount > _layerCapacity)
    {
        _telemetry.invalidLayerIndices = static_cast<std::uint32_t>(terrain.textureIndices.size());
        return false;
    }
    for (std::uint16_t entry : terrain.textureIndices)
    {
        const std::uint32_t layer = entry & 0x7fffu; // high bit is ClampU|ClampV, never a layer bit.
        if (layer >= layerCount || layer >= _layerCapacity)
            ++_telemetry.invalidLayerIndices;
    }
    return _telemetry.invalidLayerIndices == 0;
}

bool TerrainVK::CreateGrid()
{
    std::vector<GridVertex> vertices;
    std::vector<std::uint16_t> indices;
    const auto addVertex = [&](float x, float z, float skirt) { vertices.push_back({x, z, skirt}); };
    for (std::uint32_t z = 0; z <= kGridN; ++z)
        for (std::uint32_t x = 0; x <= kGridN; ++x)
            addVertex(float(x) / kGridN, float(z) / kGridN, 0.0f);
    const auto base = [](std::uint32_t x, std::uint32_t z) { return z * (kGridN + 1) + x; };
    const auto quad = [&](std::uint16_t a, std::uint16_t b, std::uint16_t c, std::uint16_t d)
    {
        indices.insert(indices.end(), {a, b, c, a, c, d});
    };
    for (std::uint32_t z = 0; z < kGridN; ++z)
        for (std::uint32_t x = 0; x < kGridN; ++x)
            quad(base(x, z), base(x + 1, z), base(x + 1, z + 1), base(x, z + 1));
    // Duplicate each boundary vertex at skirt=1 and stitch it to the surface.
    std::vector<std::uint16_t> edge;
    for (std::uint32_t x = 0; x <= kGridN; ++x) edge.push_back(base(x, 0));
    for (std::uint32_t z = 1; z <= kGridN; ++z) edge.push_back(base(kGridN, z));
    for (std::uint32_t x = kGridN; x-- > 0;) edge.push_back(base(x, kGridN));
    for (std::uint32_t z = kGridN; z-- > 1;) edge.push_back(base(0, z));
    const std::uint16_t skirtBase = static_cast<std::uint16_t>(vertices.size());
    for (std::uint16_t i : edge) addVertex(vertices[i].x, vertices[i].z, 1.0f);
    for (std::uint16_t i = 0; i < edge.size(); ++i)
    {
        const std::uint16_t n = static_cast<std::uint16_t>((i + 1) % edge.size());
        quad(edge[i], edge[n], skirtBase + n, skirtBase + i);
    }
    if (CreateHostVisibleBuffer(_physicalDevice, _device, vertices.size() * sizeof(GridVertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                _gridVertices) != VK_SUCCESS ||
        CreateHostVisibleBuffer(_physicalDevice, _device, indices.size() * sizeof(std::uint16_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                _gridIndices) != VK_SUCCESS)
        return false;
    UploadMappedBuffer(_gridVertices, vertices.data(), vertices.size() * sizeof(GridVertex));
    UploadMappedBuffer(_gridIndices, indices.data(), indices.size() * sizeof(std::uint16_t));
    _gridIndexCount = static_cast<std::uint32_t>(indices.size());
    return true;
}

bool TerrainVK::CreateRasterPipeline(const RasterInputs& inputs)
{
    // Do not manufacture visual data just to make a pipeline green. CSM and
    // self-shadow are receiver inputs, not optional visual polish.
    if (!_ready || !inputs.Complete() || _descriptorSetLayout == VK_NULL_HANDLE)
        return false;
    DestroyRasterPipeline(_device);

    const std::array<VkDescriptorSetLayout, 3> layouts = {
        inputs.frameDescriptorSetLayout, _descriptorSetLayout, inputs.visualDescriptorSetLayout};
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = static_cast<std::uint32_t>(layouts.size());
    layoutInfo.pSetLayouts = layouts.data();
    if (vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &_rasterPipelineLayout) != VK_SUCCESS)
        return false;

    std::vector<std::uint32_t> vertexSpirv, fragmentSpirv;
    std::string error;
    if (!CompileEmbeddedGlsl(kTerrainVertexShader, VK_SHADER_STAGE_VERTEX_BIT, vertexSpirv, error) ||
        !CompileEmbeddedGlsl(kTerrainFragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, fragmentSpirv, error))
    {
        DestroyRasterPipeline(_device);
        return false;
    }
    VkShaderModule vertexModule = VK_NULL_HANDLE;
    VkShaderModule fragmentModule = VK_NULL_HANDLE;
    if (!CreateEmbeddedShaderModule(_device, vertexSpirv, vertexModule) ||
        !CreateEmbeddedShaderModule(_device, fragmentSpirv, fragmentModule))
    {
        if (vertexModule) vkDestroyShaderModule(_device, vertexModule, nullptr);
        if (fragmentModule) vkDestroyShaderModule(_device, fragmentModule, nullptr);
        DestroyRasterPipeline(_device);
        return false;
    }

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertexModule;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragmentModule;
    stages[1].pName = "main";

    const std::array<VkVertexInputBindingDescription, 2> bindings = {{
        {0, sizeof(GridVertex), VK_VERTEX_INPUT_RATE_VERTEX},
        {1, sizeof(NodeInstance), VK_VERTEX_INPUT_RATE_INSTANCE},
    }};
    const std::array<VkVertexInputAttributeDescription, 4> attributes = {{
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GridVertex, x)},
        {1, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(NodeInstance, originX)},
        {2, 1, VK_FORMAT_R32_UINT, offsetof(NodeInstance, lod)},
        {3, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(NodeInstance, morphStart)},
    }};
    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = static_cast<std::uint32_t>(bindings.size());
    vertexInput.pVertexBindingDescriptions = bindings.data();
    vertexInput.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size());
    vertexInput.pVertexAttributeDescriptions = attributes.data();
    VkPipelineInputAssemblyStateCreateInfo assembly{};
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkViewport viewport{0.0f, static_cast<float>(inputs.extent.height), static_cast<float>(inputs.extent.width),
                        -static_cast<float>(inputs.extent.height), 0.0f, 1.0f};
    VkRect2D scissor{{0, 0}, inputs.extent};
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    VkPipelineRasterizationStateCreateInfo rasterizer =
        BuildRasterizationState(render::CullMode::Back, render::FrontFaceMode::CW);
    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth = BuildDepthStencilState(render::DepthMode::Normal);
    VkPipelineColorBlendAttachmentState blend = BuildColorBlendAttachmentState(render::BlendMode::Opaque);
    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &blend;
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &assembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pDepthStencilState = &depth;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.layout = _rasterPipelineLayout;
    pipelineInfo.renderPass = inputs.renderPass;
    const VkResult result = vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_rasterPipeline);
    vkDestroyShaderModule(_device, fragmentModule, nullptr);
    vkDestroyShaderModule(_device, vertexModule, nullptr);
    if (result != VK_SUCCESS)
    {
        DestroyRasterPipeline(_device);
        return false;
    }
    return true;
}

void TerrainVK::DestroyRasterPipeline(VkDevice device)
{
    if (device && _rasterPipeline) vkDestroyPipeline(device, _rasterPipeline, nullptr);
    if (device && _rasterPipelineLayout) vkDestroyPipelineLayout(device, _rasterPipelineLayout, nullptr);
    _rasterPipeline = VK_NULL_HANDLE;
    _rasterPipelineLayout = VK_NULL_HANDLE;
}

bool TerrainVK::RecordInstancedGridDraw(VkCommandBuffer commandBuffer) const
{
    if (commandBuffer == VK_NULL_HANDLE || _gridVertices.buffer == VK_NULL_HANDLE || _gridIndices.buffer == VK_NULL_HANDLE ||
        _instances.buffer == VK_NULL_HANDLE || _gridIndexCount == 0 || _visible.empty())
        return false;
    const VkBuffer buffers[] = {_gridVertices.buffer, _instances.buffer};
    const VkDeviceSize offsets[] = {0, 0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 2, buffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, _gridIndices.buffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, _gridIndexCount, static_cast<std::uint32_t>(_visible.size()), 0, 0, 0);
    return true;
}

bool TerrainVK::RecreateMapImages(const render::frame::TerrainOpaque& terrain)
{
    DestroyImage(_device, _heightmap); DestroyImage(_device, _indexMap); DestroyImage(_device, _jitterMap);
    const VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (CreateImage2D(_physicalDevice, _device, terrain.heightWidth, terrain.heightHeight, 1, VK_FORMAT_R32_SFLOAT, usage,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _heightmap) != VK_SUCCESS ||
        CreateImage2D(_physicalDevice, _device, terrain.indexWidth, terrain.indexHeight, 1, VK_FORMAT_R16_UINT, usage,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _indexMap) != VK_SUCCESS ||
        CreateImage2D(_physicalDevice, _device, terrain.indexWidth, terrain.indexHeight, 1, VK_FORMAT_R8G8_SNORM, usage,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _jitterMap) != VK_SUCCESS)
        return false;
    return UploadImage2D(_physicalDevice, _device, _commandPool, _queue, _heightmap, terrain.heights.data(),
                         terrain.heights.size() * sizeof(float)) == VK_SUCCESS &&
           UploadImage2D(_physicalDevice, _device, _commandPool, _queue, _indexMap, terrain.textureIndices.data(),
                         terrain.textureIndices.size() * sizeof(std::uint16_t)) == VK_SUCCESS &&
           UploadImage2D(_physicalDevice, _device, _commandPool, _queue, _jitterMap, terrain.jitterOffsets.data(),
                         terrain.jitterOffsets.size() * sizeof(std::int8_t)) == VK_SUCCESS;
}

bool TerrainVK::Upload(const render::frame::TerrainOpaque& terrain)
{
    if (!_ready || !terrain.Valid()) return false;
    if (!ValidateLayerIndices(terrain)) return false;
    const bool needsRebuild = _revision.NeedsRebuild(terrain.revision);
    if (needsRebuild && (!RecreateMapImages(terrain) || !UpdateStaticDescriptors())) return false;
    _params.landGrid = terrain.landGrid; _params.terrainGrid = terrain.terrainGrid;
    _params.heightWidth = terrain.heightWidth; _params.heightHeight = terrain.heightHeight;
    _params.landRange = terrain.indexWidth; _params.layerCount = static_cast<std::uint32_t>(terrain.textureLayers.size());
    _params.seaLevel = terrain.seaLevel;
    UploadMappedBuffer(_paramsBuffer, &_params, sizeof(_params));
    if (needsRebuild)
    {
        auto bounds = [&](int ox, int oz, int span, float& mn, float& mx)
        {
            for (int z = oz; z <= oz + span; ++z) for (int x = ox; x <= ox + span; ++x)
            {
                const int cx = std::clamp(x, 0, int(terrain.heightWidth) - 1);
                const int cz = std::clamp(z, 0, int(terrain.heightHeight) - 1);
                const float h = terrain.heights[std::size_t(cz) * terrain.heightWidth + cx]; mn = std::min(mn, h); mx = std::max(mx, h);
            }
        };
        const int rootTexels = CdlodRootTexels(int(terrain.heightWidth), int(kGridN));
        BuildCdlodTree(rootTexels, 0, 0, terrain.terrainGrid, kGridN, bounds, _tree, _root, _levels, _leafSize);
        ComputeCdlodRanges(_leafSize * 4.0f, 2.0f, _levels, _ranges);
        _revision.Commit(terrain.revision);
    }
    return true;
}

bool TerrainVK::UpdateLayerDescriptors(const std::vector<LayerBinding>& layers)
{
    _telemetry.boundLayers = 0;
    _telemetry.fallbackLayers = 0;
    _telemetry.invalidLayers = 0;
    if (!_ready || !_descriptorSet || layers.empty() || layers.size() > _layerCapacity)
        return false;

    std::vector<VkDescriptorImageInfo> images;
    images.reserve(layers.size());
    for (const LayerBinding& layer : layers)
    {
        if (!layer.sourceValid)
            ++_telemetry.invalidLayers;
        if (layer.usesFallback)
            ++_telemetry.fallbackLayers;
        if (layer.image.imageView == VK_NULL_HANDLE || layer.image.sampler == VK_NULL_HANDLE)
            return false;
        images.push_back(layer.image);
    }

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _descriptorSet;
    write.dstBinding = kTextureLayersBinding;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = static_cast<std::uint32_t>(images.size());
    write.pImageInfo = images.data();
    vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);
    _telemetry.boundLayers = static_cast<std::uint32_t>(images.size());
    return true;
}

void TerrainVK::Select(float cameraX, float cameraY, float cameraZ, float x0, float z0, float x1, float z1)
{
    _visible.clear();
    if (_root < 0) return;
    SelectCdlod(_tree, _root, _levels - 1, cameraX, cameraY, cameraZ, _ranges, 0.5f,
                [&](const CdlodNode& n) { return n.originX + n.size > x0 && n.originX < x1 && n.originZ + n.size > z0 && n.originZ < z1; },
                [&](const CdlodSelection& n) { _visible.push_back({n.originX, n.originZ, n.size, std::uint32_t(n.level), n.morphStart, n.morphEnd}); });
    if (_visible.size() <= 8192) UploadMappedBuffer(_instances, _visible.data(), _visible.size() * sizeof(NodeInstance));
}

void TerrainVK::Destroy(VkDevice device)
{
    _ready = false; _visible.clear(); _tree.clear(); _ranges.clear(); _revision.Invalidate();
    DestroyRasterPipeline(device);
    DestroyDescriptorResources(device);
    DestroyBuffer(device, _gridVertices); DestroyBuffer(device, _gridIndices); DestroyBuffer(device, _instances); DestroyBuffer(device, _paramsBuffer);
    DestroyImage(device, _heightmap); DestroyImage(device, _indexMap); DestroyImage(device, _jitterMap);
}

void TerrainVK::DestroyDescriptorResources(VkDevice device)
{
    if (!device)
        return;
    if (_heightSampler) vkDestroySampler(device, _heightSampler, nullptr);
    if (_indexSampler) vkDestroySampler(device, _indexSampler, nullptr);
    if (_jitterSampler) vkDestroySampler(device, _jitterSampler, nullptr);
    _heightSampler = _indexSampler = _jitterSampler = VK_NULL_HANDLE;
    if (_descriptorPool) vkDestroyDescriptorPool(device, _descriptorPool, nullptr);
    _descriptorPool = VK_NULL_HANDLE;
    _descriptorSet = VK_NULL_HANDLE;
    if (_descriptorSetLayout) vkDestroyDescriptorSetLayout(device, _descriptorSetLayout, nullptr);
    _descriptorSetLayout = VK_NULL_HANDLE;
    _layerCapacity = 0;
    _telemetry = {};
    _gridIndexCount = 0;
}
} // namespace Poseidon::vk
