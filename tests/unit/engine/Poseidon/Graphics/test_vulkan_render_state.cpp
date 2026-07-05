#include <catch2/catch_test_macros.hpp>

#include <PoseidonVK/RenderStateVK.hpp>

TEST_CASE("Vulkan render state maps cull and winding descriptors", "[vulkan][render-state]")
{
    CHECK(Poseidon::vk::ToVkCullMode(Poseidon::render::CullMode::Back) == VK_CULL_MODE_BACK_BIT);
    CHECK(Poseidon::vk::ToVkCullMode(Poseidon::render::CullMode::Front) == VK_CULL_MODE_FRONT_BIT);
    CHECK(Poseidon::vk::ToVkCullMode(Poseidon::render::CullMode::None) == VK_CULL_MODE_NONE);

    CHECK(Poseidon::vk::ToVkFrontFace(Poseidon::render::FrontFaceMode::CW) == VK_FRONT_FACE_CLOCKWISE);
    CHECK(Poseidon::vk::ToVkFrontFace(Poseidon::render::FrontFaceMode::CCW) ==
          VK_FRONT_FACE_COUNTER_CLOCKWISE);

    const VkPipelineRasterizationStateCreateInfo raster =
        Poseidon::vk::BuildRasterizationState(Poseidon::render::CullMode::None,
                                              Poseidon::render::FrontFaceMode::CW);
    CHECK(raster.sType == VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
    CHECK(raster.polygonMode == VK_POLYGON_MODE_FILL);
    CHECK(raster.cullMode == VK_CULL_MODE_NONE);
    CHECK(raster.frontFace == VK_FRONT_FACE_CLOCKWISE);
    CHECK(raster.lineWidth == 1.0f);
}

TEST_CASE("Vulkan render state maps depth descriptors", "[vulkan][render-state]")
{
    {
        const VkPipelineDepthStencilStateCreateInfo state =
            Poseidon::vk::BuildDepthStencilState(Poseidon::render::DepthMode::Normal);
        CHECK(state.depthTestEnable == VK_TRUE);
        CHECK(state.depthWriteEnable == VK_TRUE);
        CHECK(state.depthCompareOp == VK_COMPARE_OP_LESS_OR_EQUAL);
        CHECK(state.stencilTestEnable == VK_FALSE);
    }
    {
        const VkPipelineDepthStencilStateCreateInfo state =
            Poseidon::vk::BuildDepthStencilState(Poseidon::render::DepthMode::ReadOnly);
        CHECK(state.depthTestEnable == VK_TRUE);
        CHECK(state.depthWriteEnable == VK_FALSE);
        CHECK(state.depthCompareOp == VK_COMPARE_OP_LESS_OR_EQUAL);
    }
    {
        const VkPipelineDepthStencilStateCreateInfo state =
            Poseidon::vk::BuildDepthStencilState(Poseidon::render::DepthMode::Disabled);
        CHECK(state.depthTestEnable == VK_TRUE);
        CHECK(state.depthWriteEnable == VK_FALSE);
        CHECK(state.depthCompareOp == VK_COMPARE_OP_ALWAYS);
    }
    {
        const VkPipelineDepthStencilStateCreateInfo state =
            Poseidon::vk::BuildDepthStencilState(Poseidon::render::DepthMode::Shadow);
        CHECK(state.depthTestEnable == VK_TRUE);
        CHECK(state.depthWriteEnable == VK_FALSE);
        CHECK(state.stencilTestEnable == VK_TRUE);
        CHECK(state.front.compareOp == VK_COMPARE_OP_EQUAL);
        CHECK(state.front.passOp == VK_STENCIL_OP_INCREMENT_AND_CLAMP);
        CHECK(state.front.reference == 0);
        CHECK(state.back.passOp == VK_STENCIL_OP_INCREMENT_AND_CLAMP);
    }
}

TEST_CASE("Vulkan render state maps blend descriptors", "[vulkan][render-state]")
{
    {
        const VkPipelineColorBlendAttachmentState state =
            Poseidon::vk::BuildColorBlendAttachmentState(Poseidon::render::BlendMode::Opaque);
        CHECK(state.blendEnable == VK_FALSE);
        CHECK(state.srcColorBlendFactor == VK_BLEND_FACTOR_ONE);
        CHECK(state.dstColorBlendFactor == VK_BLEND_FACTOR_ZERO);
    }
    {
        const VkPipelineColorBlendAttachmentState state =
            Poseidon::vk::BuildColorBlendAttachmentState(Poseidon::render::BlendMode::AlphaBlend);
        CHECK(state.blendEnable == VK_TRUE);
        CHECK(state.srcColorBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA);
        CHECK(state.dstColorBlendFactor == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    }
    {
        const VkPipelineColorBlendAttachmentState state =
            Poseidon::vk::BuildColorBlendAttachmentState(Poseidon::render::BlendMode::Additive);
        CHECK(state.blendEnable == VK_TRUE);
        CHECK(state.srcColorBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA);
        CHECK(state.dstColorBlendFactor == VK_BLEND_FACTOR_ONE);
    }
    {
        const VkPipelineColorBlendAttachmentState state =
            Poseidon::vk::BuildColorBlendAttachmentState(Poseidon::render::BlendMode::Shadow);
        CHECK(state.blendEnable == VK_TRUE);
        CHECK(state.srcColorBlendFactor == VK_BLEND_FACTOR_ZERO);
        CHECK(state.dstColorBlendFactor == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    }
}

TEST_CASE("Vulkan sampler state maps filter descriptors", "[vulkan][render-state]")
{
    using Poseidon::render::SamplerFilter;

    CHECK(Poseidon::vk::ToVkMagFilter(SamplerFilter::Point) == VK_FILTER_NEAREST);
    CHECK(Poseidon::vk::ToVkMagFilter(SamplerFilter::Linear) == VK_FILTER_LINEAR);
    CHECK(Poseidon::vk::ToVkMinFilter(SamplerFilter::Point) == VK_FILTER_NEAREST);
    CHECK(Poseidon::vk::ToVkMinFilter(SamplerFilter::Linear) == VK_FILTER_LINEAR);
    CHECK(Poseidon::vk::ToVkMipmapMode(SamplerFilter::Point) == VK_SAMPLER_MIPMAP_MODE_NEAREST);
    CHECK(Poseidon::vk::ToVkMipmapMode(SamplerFilter::Linear) == VK_SAMPLER_MIPMAP_MODE_LINEAR);
}

TEST_CASE("Vulkan sampler state maps clamp descriptors", "[vulkan][render-state]")
{
    CHECK(Poseidon::vk::ToVkAddressMode(true) == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    CHECK(Poseidon::vk::ToVkAddressMode(false) == VK_SAMPLER_ADDRESS_MODE_REPEAT);
}

TEST_CASE("Vulkan sampler create info matches the sampler descriptor", "[vulkan][render-state]")
{
    using Poseidon::render::SamplerFilter;
    using Poseidon::render::SamplerMode;

    {
        const SamplerMode sampler{SamplerFilter::Linear, false, false};
        const VkSamplerCreateInfo info = Poseidon::vk::BuildSamplerState(sampler);
        CHECK(info.sType == VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
        CHECK(info.magFilter == VK_FILTER_LINEAR);
        CHECK(info.minFilter == VK_FILTER_LINEAR);
        CHECK(info.mipmapMode == VK_SAMPLER_MIPMAP_MODE_LINEAR);
        CHECK(info.addressModeU == VK_SAMPLER_ADDRESS_MODE_REPEAT);
        CHECK(info.addressModeV == VK_SAMPLER_ADDRESS_MODE_REPEAT);
        CHECK(info.addressModeW == VK_SAMPLER_ADDRESS_MODE_REPEAT);
        CHECK(info.unnormalizedCoordinates == VK_FALSE);
        CHECK(info.maxLod == VK_LOD_CLAMP_NONE);
    }
    {
        const SamplerMode sampler{SamplerFilter::Point, true, true};
        const VkSamplerCreateInfo info = Poseidon::vk::BuildSamplerState(sampler);
        CHECK(info.magFilter == VK_FILTER_NEAREST);
        CHECK(info.minFilter == VK_FILTER_NEAREST);
        CHECK(info.mipmapMode == VK_SAMPLER_MIPMAP_MODE_NEAREST);
        CHECK(info.addressModeU == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
        CHECK(info.addressModeV == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
        CHECK(info.addressModeW == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    }
    {
        const SamplerMode sampler{SamplerFilter::Linear, true, false};
        const VkSamplerCreateInfo info = Poseidon::vk::BuildSamplerState(sampler);
        CHECK(info.addressModeU == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
        CHECK(info.addressModeV == VK_SAMPLER_ADDRESS_MODE_REPEAT);
        CHECK(info.addressModeW == VK_SAMPLER_ADDRESS_MODE_REPEAT);
    }
}
