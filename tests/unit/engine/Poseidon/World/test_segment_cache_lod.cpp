#include <catch2/catch_test_macros.hpp>

#include <Poseidon/World/Terrain/LandscapeLod.hpp>

#include <memory>

using namespace Poseidon;

// I-25 / B-033: Terrain LOD cache stale on LOD transition.
//
// `LandCache::Segment` / `Fill` previously had cache-hit paths
// that returned a previously-cached segment without re-checking
// the LOD level — roads vanished as the camera moved through
// LOD bands.  The fix moved freshness checking into
// `SegmentCache::Lookup` (templated, no terrain-engine
// dependencies) — these tests drive that class directly with a
// trivial Entry type so the cache-key-includes-LOD contract is
// pinned at the unit-test level.

namespace
{
struct TestSlot
{
    int xBeg = 0;
    int zBeg = 0;
    int lodLevel = 0;
    float lastUsed = 0.0f;
    int tag = 0; // identifies which generation produced this slot
};

// Helper that emulates the LandCache::Segment lambda.
struct GeneratorCounter
{
    int callCount = 0;
    int nextTag = 100;

    auto fn()
    {
        return [this](int x, int z, int lod, float time) -> std::unique_ptr<TestSlot>
        {
            ++callCount;
            auto slot = std::make_unique<TestSlot>();
            slot->xBeg = x;
            slot->zBeg = z;
            slot->lodLevel = lod;
            slot->lastUsed = time;
            slot->tag = nextTag++;
            return slot;
        };
    }
};
} // namespace

TEST_CASE("SegmentCache: first lookup is a miss and runs the generator", "[World][SegmentCache][I-25]")
{
    SegmentCache<TestSlot> cache(/*maxN*/ 16);
    GeneratorCounter gen;

    auto r = cache.Lookup(/*x*/ 100, /*z*/ 200, /*lod*/ 0, /*time*/ 1.0f, gen.fn());

    REQUIRE(gen.callCount == 1);
    REQUIRE_FALSE(r.wasHit);
    REQUIRE_FALSE(r.wasLodEvict);
    REQUIRE(r.entry != nullptr);
    REQUIRE(r.entry->xBeg == 100);
    REQUIRE(r.entry->zBeg == 200);
    REQUIRE(r.entry->lodLevel == 0);
}

TEST_CASE("SegmentCache: same key, same LOD is a hit and doesn't re-run the generator", "[World][SegmentCache][I-25]")
{
    SegmentCache<TestSlot> cache(/*maxN*/ 16);
    GeneratorCounter gen;

    auto first = cache.Lookup(100, 200, /*lod*/ 0, 1.0f, gen.fn());
    const int firstTag = first.entry->tag;

    auto second = cache.Lookup(100, 200, /*lod*/ 0, 2.0f, gen.fn());

    REQUIRE(gen.callCount == 1); // not re-generated
    REQUIRE(second.wasHit);
    REQUIRE_FALSE(second.wasLodEvict);
    REQUIRE(second.entry->tag == firstTag);  // same instance
    REQUIRE(second.entry->lastUsed == 2.0f); // promoted to MRU with new time
}

TEST_CASE("SegmentCache: same key, different LOD evicts and regenerates (B-033)", "[World][SegmentCache][I-25]")
{
    SegmentCache<TestSlot> cache(/*maxN*/ 16);
    GeneratorCounter gen;

    auto first = cache.Lookup(100, 200, /*lod*/ 0, 1.0f, gen.fn());
    const int firstTag = first.entry->tag;
    REQUIRE(first.entry->lodLevel == 0);

    // Camera moves between LOD bands — same (x, z) key, new LOD.
    auto second = cache.Lookup(100, 200, /*lod*/ 1, 2.0f, gen.fn());

    REQUIRE(gen.callCount == 2); // regenerated for new LOD
    REQUIRE_FALSE(second.wasHit);
    REQUIRE(second.wasLodEvict);          // evict-path was taken
    REQUIRE(second.entry->lodLevel == 1); // fresh entry carries new LOD
    REQUIRE(second.entry->tag != firstTag);
}

TEST_CASE("SegmentCache: different keys are independent", "[World][SegmentCache][I-25]")
{
    SegmentCache<TestSlot> cache(/*maxN*/ 16);
    GeneratorCounter gen;

    cache.Lookup(100, 200, 0, 1.0f, gen.fn());
    cache.Lookup(300, 400, 0, 1.0f, gen.fn());

    REQUIRE(gen.callCount == 2);
    REQUIRE(cache.Size() == 2);

    auto hit1 = cache.Lookup(100, 200, 0, 2.0f, gen.fn());
    auto hit2 = cache.Lookup(300, 400, 0, 2.0f, gen.fn());

    REQUIRE(hit1.wasHit);
    REQUIRE(hit2.wasHit);
    REQUIRE(gen.callCount == 2);
}

TEST_CASE("SegmentCache: LOD transition does not pollute neighbour entries", "[World][SegmentCache][I-25]")
{
    SegmentCache<TestSlot> cache(/*maxN*/ 16);
    GeneratorCounter gen;

    cache.Lookup(100, 200, /*lod*/ 0, 1.0f, gen.fn());
    cache.Lookup(300, 400, /*lod*/ 0, 1.0f, gen.fn());

    // Evict-via-LOD on the first key only.
    auto r = cache.Lookup(100, 200, /*lod*/ 1, 2.0f, gen.fn());
    REQUIRE(r.wasLodEvict);

    // Neighbour entry must still be a hit at its original LOD.
    auto neighbour = cache.Lookup(300, 400, /*lod*/ 0, 3.0f, gen.fn());
    REQUIRE(neighbour.wasHit);
    REQUIRE_FALSE(neighbour.wasLodEvict);
    REQUIRE(neighbour.entry->lodLevel == 0);
}

TEST_CASE("SegmentCache: LRU eviction triggers on capacity overflow", "[World][SegmentCache][I-25]")
{
    SegmentCache<TestSlot> cache(/*maxN*/ 2);
    GeneratorCounter gen;

    cache.Lookup(100, 100, 0, 1.0f, gen.fn()); // slot A
    cache.Lookup(200, 200, 0, 2.0f, gen.fn()); // slot B (full)
    cache.Lookup(300, 300, 0, 3.0f, gen.fn()); // slot C — evicts A (LRU)

    REQUIRE(cache.Size() == 2);
    REQUIRE(cache.Find(100, 100) == nullptr); // A gone
    REQUIRE(cache.Find(200, 200) != nullptr);
    REQUIRE(cache.Find(300, 300) != nullptr);
}

TEST_CASE("SegmentCache: Find returns null for never-cached key", "[World][SegmentCache][I-25]")
{
    SegmentCache<TestSlot> cache(/*maxN*/ 16);

    REQUIRE(cache.Find(999, 999) == nullptr);
}
