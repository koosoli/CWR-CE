// Unit tests for Poseidon::Handle<T>.  Pins:
//   - Default constructor produces an invalid handle
//   - IsValid is true iff generation != 0
//   - Equality + inequality compare both slot and generation
//   - Hash is well-defined (usable as unordered_map key)
//   - Two handles into different slots are not equal
//   - Same slot at different generations is not equal (stale-handle case)
//   - Handles of different T types do not compile as the same type
//     (compile-time safety check — only checkable via static_assert
//     on `std::is_same`, since interchangeability would fail compile)

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Asset/Cache/Handle.hpp>

#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace
{
struct TestTexture;
struct TestShape;
} // namespace

using TextureHandle = Poseidon::Handle<TestTexture>;
using ShapeHandle = Poseidon::Handle<TestShape>;

TEST_CASE("AssetHandle default construction is invalid", "[asset_handle]")
{
    TextureHandle h;
    REQUIRE_FALSE(h.IsValid());
    REQUIRE(h == TextureHandle::Invalid());
}

TEST_CASE("AssetHandle with non-zero generation is valid", "[asset_handle]")
{
    TextureHandle h(0, 1);
    REQUIRE(h.IsValid());
    REQUIRE(h.Slot() == 0);
    REQUIRE(h.Generation() == 1);
}

TEST_CASE("AssetHandle with zero generation is invalid even if slot is non-zero", "[asset_handle]")
{
    TextureHandle h(42, 0);
    REQUIRE_FALSE(h.IsValid());
}

TEST_CASE("AssetHandle equality compares both slot and generation", "[asset_handle]")
{
    TextureHandle a(5, 1);
    TextureHandle b(5, 1);
    TextureHandle c(5, 2); // same slot, newer generation (stale-after-reuse case)
    TextureHandle d(6, 1); // different slot

    REQUIRE(a == b);
    REQUIRE(a != c);
    REQUIRE(a != d);
    REQUIRE(c != d);
}

TEST_CASE("AssetHandle hash works as unordered_map key", "[asset_handle][hash]")
{
    std::unordered_map<TextureHandle, int> m;
    m[TextureHandle(1, 1)] = 11;
    m[TextureHandle(1, 2)] = 12;
    m[TextureHandle(2, 1)] = 21;

    REQUIRE(m.size() == 3);
    REQUIRE(m[TextureHandle(1, 1)] == 11);
    REQUIRE(m[TextureHandle(1, 2)] == 12);
    REQUIRE(m[TextureHandle(2, 1)] == 21);
    REQUIRE(m.find(TextureHandle(99, 99)) == m.end());
}

TEST_CASE("AssetHandle is trivially copyable + small", "[asset_handle][repr]")
{
    // Cheap to pass by value.  Two uint32_t = 8 bytes.
    static_assert(std::is_trivially_copyable_v<TextureHandle>);
    static_assert(sizeof(TextureHandle) == 8);
}

TEST_CASE("AssetHandle of different types are distinct types", "[asset_handle][types]")
{
    // The whole point of templating Handle<T> on T is compile-time
    // safety.  This static_assert proves that mixing handle types
    // doesn't compile silently.
    static_assert(!std::is_same_v<TextureHandle, ShapeHandle>, "Handle<T> with different T must be distinct types");
}
