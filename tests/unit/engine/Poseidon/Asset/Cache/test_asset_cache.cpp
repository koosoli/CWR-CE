// Unit tests for Poseidon::AssetCache<T>.  Pins (in order of
// importance for the upcoming caller migrations):
//
//   - Find on missing key returns invalid handle, bumps misses
//   - Find on present key returns valid handle, bumps hits
//   - Insert of new key allocates slot, bumps inserts + size
//   - Insert of existing key is idempotent (returns existing handle,
//     does NOT replace; counted as hit, not insert)
//   - Get(stale handle) returns nullptr after Remove + Insert recycle
//   - Slot reuse preserves O(1) amortised insert
//   - Case-insensitive keying for StringKey (matches BankArray)
//   - ForEach iterates every live entry exactly once
//   - Clear evicts everything; stats record evictions
//   - shared_ptr storage cleanup: removed value's refcount drops to
//     zero, destructor runs
//   - Stats fields are independently observable
//
// Tests use std::string keys + std::shared_ptr storage so they
// compile without engine-specific dependencies.  Engine consumers
// will templatize differently (Ref<T> + RStringB) but the contract
// is identical.

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Asset/Cache/AssetCache.hpp>

#include <memory>
#include <set>
#include <string>

namespace
{

// Test value type: tracks construction / destruction count so tests
// can assert lifetime correctness.
struct Resource
{
    static int liveCount;
    int id;

    explicit Resource(int i) : id(i) { ++liveCount; }
    ~Resource() { --liveCount; }
    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;
};

int Resource::liveCount = 0;

using Cache = ::Poseidon::AssetCache<Resource>;

std::shared_ptr<Resource> Make(int id)
{
    return std::make_shared<Resource>(id);
}

} // namespace

TEST_CASE("AssetCache Find on missing key returns invalid handle", "[asset_cache]")
{
    Cache c;
    auto h = c.Find("absent");
    REQUIRE_FALSE(h.IsValid());
    REQUIRE(c.GetStats().misses == 1);
    REQUIRE(c.GetStats().hits == 0);
}

TEST_CASE("AssetCache Insert allocates slot and returns valid handle", "[asset_cache]")
{
    Cache c;
    auto h = c.Insert("foo", Make(42));
    REQUIRE(h.IsValid());
    REQUIRE(c.Size() == 1);
    REQUIRE(c.GetStats().inserts == 1);

    Resource* r = c.Get(h);
    REQUIRE(r != nullptr);
    REQUIRE(r->id == 42);
}

TEST_CASE("AssetCache Find after Insert returns same handle, bumps hits", "[asset_cache]")
{
    Cache c;
    auto h1 = c.Insert("foo", Make(1));
    auto h2 = c.Find("foo");
    REQUIRE(h1 == h2);
    REQUIRE(c.GetStats().hits == 1);
    REQUIRE(c.GetStats().misses == 0);
}

TEST_CASE("AssetCache Insert of existing key is idempotent, drops the new value", "[asset_cache]")
{
    Cache c;
    auto first = c.Insert("foo", Make(1));
    int liveBefore = Resource::liveCount;

    // The new shared_ptr would be the only owner of Resource(2);
    // Insert must drop it on the floor because "foo" is already
    // present.  After Insert returns, the temporary shared_ptr
    // goes out of scope and Resource(2) is destroyed.
    auto second = c.Insert("foo", Make(2));

    REQUIRE(first == second);
    REQUIRE(c.Get(second)->id == 1);            // original value still there
    REQUIRE(Resource::liveCount == liveBefore); // new value was destroyed
    REQUIRE(c.GetStats().inserts == 1);
    REQUIRE(c.GetStats().hits == 1); // the second Insert counted as hit
}

TEST_CASE("AssetCache Get(stale handle) returns nullptr after Remove + new Insert", "[asset_cache][stale]")
{
    Cache c;
    auto h1 = c.Insert("foo", Make(1));
    c.Remove(h1);
    REQUIRE(c.Get(h1) == nullptr); // gone

    auto h2 = c.Insert("bar", Make(2));          // recycles the slot from h1
    REQUIRE(h2.Slot() == h1.Slot());             // same slot
    REQUIRE(h2.Generation() != h1.Generation()); // different generation
    REQUIRE(c.Get(h1) == nullptr);               // h1 still stale
    REQUIRE(c.Get(h2) != nullptr);               // h2 lives
    REQUIRE(c.Get(h2)->id == 2);
}

TEST_CASE("AssetCache Remove decrements size and bumps evictions", "[asset_cache]")
{
    Cache c;
    auto h = c.Insert("foo", Make(1));
    REQUIRE(c.Size() == 1);
    c.Remove(h);
    REQUIRE(c.Size() == 0);
    REQUIRE(c.GetStats().evictions == 1);
    REQUIRE(c.Find("foo").IsValid() == false);
}

TEST_CASE("AssetCache Remove by key", "[asset_cache]")
{
    Cache c;
    c.Insert("foo", Make(1));
    REQUIRE(c.Size() == 1);
    c.Remove("foo");
    REQUIRE(c.Size() == 0);
    REQUIRE(c.GetStats().evictions == 1);
}

TEST_CASE("AssetCache Clear evicts everything", "[asset_cache]")
{
    Cache c;
    for (int i = 0; i < 5; ++i)
    {
        c.Insert("k" + std::to_string(i), Make(i));
    }
    REQUIRE(c.Size() == 5);
    REQUIRE(Resource::liveCount == 5);

    c.Clear();
    REQUIRE(c.Size() == 0);
    REQUIRE(Resource::liveCount == 0); // all destroyed
    REQUIRE(c.GetStats().evictions == 5);
}

TEST_CASE("AssetCache keys are case-insensitive (matches BankArray)", "[asset_cache][keys]")
{
    Cache c;
    c.Insert("Foo.PAA", Make(1));
    REQUIRE(c.Find("foo.paa").IsValid());
    REQUIRE(c.Find("FOO.PAA").IsValid());
    REQUIRE(c.Find("Foo.PAA").IsValid());
    REQUIRE(c.Lookup("foo.paa")->id == 1);
}

TEST_CASE("AssetCache ForEach iterates every live entry exactly once", "[asset_cache][iterate]")
{
    Cache c;
    c.Insert("a", Make(1));
    c.Insert("b", Make(2));
    c.Insert("c", Make(3));

    std::set<int> seen;
    c.ForEach([&](auto /*h*/, const auto& /*key*/, Resource& r) { seen.insert(r.id); });
    REQUIRE(seen == std::set<int>{1, 2, 3});
}

TEST_CASE("AssetCache ForEach skips removed entries", "[asset_cache][iterate]")
{
    Cache c;
    auto ha = c.Insert("a", Make(1));
    c.Insert("b", Make(2));
    c.Insert("c", Make(3));
    c.Remove(ha);

    std::set<int> seen;
    c.ForEach([&](auto /*h*/, const auto& /*key*/, Resource& r) { seen.insert(r.id); });
    REQUIRE(seen == std::set<int>{2, 3});
}

// Regression: ShapeBank stores Link<LODShapeWithShadow> (weak), and when
// the external Ref<> chain drops a shape its Link in the cache auto-nulls
// while the slot remains keyed.  ForEach must skip those entries instead
// of dereferencing the null and crashing.  Without the null-guard, the
// multiplayer create_game test crashed in ShapeBank::ReleaseAllVBuffers
// during InitVehicles after the briefing -> mission transition.
TEST_CASE("AssetCache ForEach skips entries whose storage adapter is null", "[asset_cache][iterate][weak]")
{
    // Storage adapter that exposes operator-> + nullptr-comparable + default-
    // constructible, but returns nullptr — modelling a weak ref whose target
    // was destroyed since insertion.
    struct NullStorage
    {
        NullStorage() = default;
        NullStorage(Resource* /*r*/) {}
        Resource* operator->() const { return nullptr; }
        explicit operator bool() const { return false; }
    };

    ::Poseidon::AssetCache<Resource, NullStorage> c;
    auto liveResource = Make(7); // keep one Resource live so the adapter ctor has a target
    c.Insert("dead", NullStorage(liveResource.get()));

    int callbackInvocations = 0;
    c.ForEach([&](auto /*h*/, const auto& /*key*/, Resource& /*r*/) { ++callbackInvocations; });

    // Slot is occupied but the storage adapter reports null — the callback
    // must not fire, otherwise we'd dereference null inside ForEach.
    REQUIRE(callbackInvocations == 0);
    REQUIRE(c.Size() == 1); // slot still counts as live
}

TEST_CASE("AssetCache Lookup convenience returns same as Find + Get", "[asset_cache]")
{
    Cache c;
    c.Insert("foo", Make(7));
    REQUIRE(c.Lookup("foo")->id == 7);
    REQUIRE(c.Lookup("absent") == nullptr);
}

TEST_CASE("AssetCache slot reuse is O(1) amortised over insert + remove cycle", "[asset_cache][stress]")
{
    Cache c;
    // 1000 churns through a fixed-name slot.  Without reuse this would
    // grow _slots forever; with reuse the slot count stays bounded.
    for (int i = 0; i < 1000; ++i)
    {
        auto h = c.Insert("churn", Make(i));
        c.Remove(h);
    }
    REQUIRE(c.Size() == 0);
    REQUIRE(c.GetStats().inserts == 1000);
    REQUIRE(c.GetStats().evictions == 1000);
    // No leaks despite 1000 inserts.
    REQUIRE(Resource::liveCount == 0);
}

TEST_CASE("AssetCache Get(handle) does not bump hits/misses", "[asset_cache][stats]")
{
    Cache c;
    auto h = c.Insert("foo", Make(1));
    const auto before = c.GetStats();
    (void)c.Get(h);
    (void)c.Get(h);
    (void)c.Get(h);
    const auto after = c.GetStats();
    REQUIRE(after.hits == before.hits);
    REQUIRE(after.misses == before.misses);
}

TEST_CASE("AssetCache GetStorage allows asset to outlive cache eviction", "[asset_cache][lifetime]")
{
    Cache c;
    auto h = c.Insert("foo", Make(99));
    std::shared_ptr<Resource> longLife = c.GetStorage(h);
    REQUIRE(longLife != nullptr);
    REQUIRE(longLife->id == 99);

    c.Remove(h);
    // After removal, the cache no longer holds the value, but our
    // shared_ptr does — Resource is still alive.
    REQUIRE(c.Get(h) == nullptr);
    REQUIRE(longLife != nullptr);
    REQUIRE(longLife->id == 99);
    REQUIRE(Resource::liveCount >= 1);
}
