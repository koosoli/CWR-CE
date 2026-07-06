#include <catch2/catch_test_macros.hpp>

#include <PoseidonVK/MeshRegistryVK.hpp>

#include <cstdint>

namespace
{

Poseidon::vk::MeshResourcesVK MakeResources(std::uint32_t id, std::uint32_t indexCount)
{
    // Cast integers to VkBuffer for test purposes — the registry only stores
    // the handle value; it never dereferences it.
    Poseidon::vk::MeshResourcesVK r;
    r.vertexBuffer = reinterpret_cast<VkBuffer>(static_cast<std::uintptr_t>(id * 2 + 1));
    r.indexBuffer = reinterpret_cast<VkBuffer>(static_cast<std::uintptr_t>(id * 2 + 2));
    r.vertexCount = indexCount * 2;
    r.indexCount = indexCount;
    return r;
}

} // namespace

TEST_CASE("Vulkan mesh registry resolves a registered mesh", "[vulkan][mesh-registry]")
{
    Poseidon::vk::MeshRegistryVK registry;
    const Poseidon::vk::MeshResourcesVK resources = MakeResources(7, 18);

    REQUIRE(registry.Register(7, resources));
    REQUIRE(registry.Size() == 1);
    REQUIRE(registry.Contains(7));

    const Poseidon::vk::MeshResourcesVK* resolved = registry.Resolve(7);
    REQUIRE(resolved != nullptr);
    CHECK(resolved->vertexBuffer == resources.vertexBuffer);
    CHECK(resolved->indexBuffer == resources.indexBuffer);
    CHECK(resolved->vertexCount == 36);
    CHECK(resolved->indexCount == 18);
    CHECK(resolved->IsValid());
}

TEST_CASE("Vulkan mesh registry returns null for unknown id", "[vulkan][mesh-registry]")
{
    Poseidon::vk::MeshRegistryVK registry;
    registry.Register(1, MakeResources(1, 6));

    CHECK(registry.Resolve(999) == nullptr);
    CHECK_FALSE(registry.Contains(999));
}

TEST_CASE("Vulkan mesh registry unregister removes the entry", "[vulkan][mesh-registry]")
{
    Poseidon::vk::MeshRegistryVK registry;
    registry.Register(3, MakeResources(3, 12));

    REQUIRE(registry.Unregister(3));
    CHECK(registry.Size() == 0);
    CHECK(registry.Resolve(3) == nullptr);
    CHECK_FALSE(registry.Unregister(3)); // already gone
}

TEST_CASE("Vulkan mesh registry overwrite replaces resources", "[vulkan][mesh-registry]")
{
    Poseidon::vk::MeshRegistryVK registry;
    registry.Register(5, MakeResources(5, 10));

    // Overwrite with a different resource set.
    const Poseidon::vk::MeshResourcesVK updated = MakeResources(5, 99);
    CHECK_FALSE(registry.Register(5, updated)); // overwrite, not insert

    const Poseidon::vk::MeshResourcesVK* resolved = registry.Resolve(5);
    REQUIRE(resolved != nullptr);
    CHECK(resolved->indexCount == 99);
    CHECK(registry.Size() == 1); // still one entry, not two
}

TEST_CASE("Vulkan mesh registry clear empties all entries", "[vulkan][mesh-registry]")
{
    Poseidon::vk::MeshRegistryVK registry;
    registry.Register(1, MakeResources(1, 3));
    registry.Register(2, MakeResources(2, 6));
    registry.Register(3, MakeResources(3, 9));
    REQUIRE(registry.Size() == 3);

    registry.Clear();
    CHECK(registry.Size() == 0);
    CHECK(registry.Resolve(1) == nullptr);
    CHECK(registry.Resolve(2) == nullptr);
    CHECK(registry.Resolve(3) == nullptr);
}

TEST_CASE("Vulkan mesh resources IsValid reflects buffer presence", "[vulkan][mesh-registry]")
{
    Poseidon::vk::MeshResourcesVK empty{};
    CHECK_FALSE(empty.IsValid());

    Poseidon::vk::MeshResourcesVK partial{};
    partial.vertexBuffer = reinterpret_cast<VkBuffer>(1);
    CHECK_FALSE(partial.IsValid()); // missing index buffer

    Poseidon::vk::MeshResourcesVK full = MakeResources(1, 6);
    CHECK(full.IsValid());
}
