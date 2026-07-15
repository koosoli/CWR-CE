#include <catch2/catch_test_macros.hpp>

#include <PoseidonVK/GpuSceneVK.hpp>

#include <array>

TEST_CASE("Vulkan GPU scene capability tiers retain a safe fallback", "[vulkan][gpu-scene]")
{
    Poseidon::vk::GpuSceneCapabilitiesVK capabilities;
    CHECK_FALSE(capabilities.GpuDrivenAvailable());
    capabilities.compute = true;
    capabilities.multiDrawIndirect = true;
    CHECK_FALSE(capabilities.GpuDrivenAvailable());
    capabilities.shaderDrawParameters = true;
    CHECK(capabilities.GpuDrivenAvailable());
    CHECK_FALSE(capabilities.drawIndirectCount); // fixed-count indirect remains valid
}

TEST_CASE("Vulkan GPU scene LOD selection chooses the furthest crossed threshold", "[vulkan][gpu-scene]")
{
    const std::array<float, 4> distances = {0.0f, 40.0f, 100.0f, 250.0f};
    CHECK(Poseidon::vk::SelectGpuSceneLod(distances, 0.0f) == 0);
    CHECK(Poseidon::vk::SelectGpuSceneLod(distances, 40.0f) == 1);
    CHECK(Poseidon::vk::SelectGpuSceneLod(distances, 180.0f) == 2);
    CHECK(Poseidon::vk::SelectGpuSceneLod(distances, 500.0f) == 3);
}

TEST_CASE("Vulkan GPU scene ABI matches Vulkan indirect commands", "[vulkan][gpu-scene]")
{
    STATIC_REQUIRE(sizeof(Poseidon::vk::GpuSceneInstanceVK) == 80);
    STATIC_REQUIRE(sizeof(VkDrawIndexedIndirectCommand) == 20);
    const Poseidon::vk::GpuSceneBatchVK batch{5, 8, 5, 2, 17, 1};
    CHECK(batch.firstInstance == batch.indirectOffset);
    CHECK(batch.countOffset == 2);
}

TEST_CASE("Vulkan GPU shadow ABI carries indirect base-instance data", "[vulkan][gpu-shadow]")
{
    STATIC_REQUIRE(sizeof(Poseidon::vk::GpuShadowInstanceVK) == 112);
    STATIC_REQUIRE(offsetof(Poseidon::vk::GpuShadowInstanceVK, world) == 16);
    STATIC_REQUIRE(offsetof(Poseidon::vk::GpuShadowInstanceVK, alphaCutoff) == 100);
    STATIC_REQUIRE(sizeof(Poseidon::vk::GpuShadowBatchVK) == 32);
    STATIC_REQUIRE(sizeof(VkDrawIndexedIndirectCommand) == 20);

    const Poseidon::vk::GpuShadowBatchVK batch{3, 7, 3, 2, 9, 11, 1, 0};
    CHECK(batch.firstInstance == batch.indirectOffset);
    CHECK(batch.alphaTested == 1);
    CHECK(batch.alphaTextureId == 11);
}
