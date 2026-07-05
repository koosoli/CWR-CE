#include <catch2/catch_test_macros.hpp>

#include <PoseidonVK/VertexLayoutVK.hpp>

#include <array>

TEST_CASE("Vulkan scene vertex layout has stable stride and offsets", "[vulkan][vertex-layout]")
{
    using namespace Poseidon::vk;

    STATIC_REQUIRE(kSceneVertexStride == 32);
    STATIC_REQUIRE(kSceneVertexPositionOffset == 0);
    STATIC_REQUIRE(kSceneVertexNormalOffset == 12);
    STATIC_REQUIRE(kSceneVertexTexcoordOffset == 24);
    STATIC_REQUIRE(kSceneVertexAttributeCount == 3);

    STATIC_REQUIRE(kSceneVertexPositionOffset + 12 == kSceneVertexNormalOffset);
    STATIC_REQUIRE(kSceneVertexNormalOffset + 12 == kSceneVertexTexcoordOffset);
    STATIC_REQUIRE(kSceneVertexTexcoordOffset + 8 == kSceneVertexStride);
}

TEST_CASE("Vulkan scene vertex locations are unique and ordered", "[vulkan][vertex-layout]")
{
    using namespace Poseidon::vk;

    STATIC_REQUIRE(kSceneVertexLocationPosition == 0);
    STATIC_REQUIRE(kSceneVertexLocationNormal == 1);
    STATIC_REQUIRE(kSceneVertexLocationTexcoord == 2);
    STATIC_REQUIRE(kSceneVertexLocationPosition < kSceneVertexAttributeCount);
    STATIC_REQUIRE(kSceneVertexLocationNormal < kSceneVertexAttributeCount);
    STATIC_REQUIRE(kSceneVertexLocationTexcoord < kSceneVertexAttributeCount);
}

TEST_CASE("Vulkan scene vertex binding description matches mesh contract", "[vulkan][vertex-layout]")
{
    const VkVertexInputBindingDescription binding = Poseidon::vk::MakeSceneVertexBindingDescription();
    CHECK(binding.binding == Poseidon::vk::kSceneVertexBinding);
    CHECK(binding.stride == Poseidon::vk::kSceneVertexStride);
    CHECK(binding.inputRate == VK_VERTEX_INPUT_RATE_VERTEX);
}

TEST_CASE("Vulkan scene vertex attribute descriptions cover pos, norm, uv", "[vulkan][vertex-layout]")
{
    const VkVertexInputAttributeDescription pos = Poseidon::vk::MakeSceneVertexPositionAttribute();
    CHECK(pos.location == Poseidon::vk::kSceneVertexLocationPosition);
    CHECK(pos.binding == Poseidon::vk::kSceneVertexBinding);
    CHECK(pos.format == VK_FORMAT_R32G32B32_SFLOAT);
    CHECK(pos.offset == Poseidon::vk::kSceneVertexPositionOffset);

    const VkVertexInputAttributeDescription norm = Poseidon::vk::MakeSceneVertexNormalAttribute();
    CHECK(norm.location == Poseidon::vk::kSceneVertexLocationNormal);
    CHECK(norm.binding == Poseidon::vk::kSceneVertexBinding);
    CHECK(norm.format == VK_FORMAT_R32G32B32_SFLOAT);
    CHECK(norm.offset == Poseidon::vk::kSceneVertexNormalOffset);

    const VkVertexInputAttributeDescription uv = Poseidon::vk::MakeSceneVertexTexcoordAttribute();
    CHECK(uv.location == Poseidon::vk::kSceneVertexLocationTexcoord);
    CHECK(uv.binding == Poseidon::vk::kSceneVertexBinding);
    CHECK(uv.format == VK_FORMAT_R32G32_SFLOAT);
    CHECK(uv.offset == Poseidon::vk::kSceneVertexTexcoordOffset);
}

TEST_CASE("Vulkan scene vertex attribute set is complete and ordered", "[vulkan][vertex-layout]")
{
    const std::array<VkVertexInputAttributeDescription, Poseidon::vk::kSceneVertexAttributeCount>
        attributes = Poseidon::vk::MakeSceneVertexAttributeDescriptions();

    REQUIRE(attributes.size() == Poseidon::vk::kSceneVertexAttributeCount);
    CHECK(attributes[0].location == Poseidon::vk::kSceneVertexLocationPosition);
    CHECK(attributes[0].format == VK_FORMAT_R32G32B32_SFLOAT);
    CHECK(attributes[1].location == Poseidon::vk::kSceneVertexLocationNormal);
    CHECK(attributes[1].format == VK_FORMAT_R32G32B32_SFLOAT);
    CHECK(attributes[2].location == Poseidon::vk::kSceneVertexLocationTexcoord);
    CHECK(attributes[2].format == VK_FORMAT_R32G32_SFLOAT);

    // Offsets must be strictly increasing and pack tightly into the stride.
    CHECK(attributes[0].offset < attributes[1].offset);
    CHECK(attributes[1].offset < attributes[2].offset);
    CHECK(attributes[2].offset + 8 == Poseidon::vk::kSceneVertexStride);
}
