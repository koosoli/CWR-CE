#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Audio/Shared/PcmCache.hpp>

using Poseidon::Audio::PcmCache;

namespace
{

PcmCache::EntryPtr MakeEntry(size_t bytes)
{
    auto e = std::make_shared<PcmCache::Entry>();
    e->uncompressedSize = static_cast<unsigned int>(bytes);
    e->pcm.assign(bytes, 0x5a);
    return e;
}

} // namespace

TEST_CASE("PcmCache: insert/find roundtrip with case- and separator-insensitive keys", "[Audio][PcmCache]")
{
    PcmCache cache;
    REQUIRE(cache.Insert("Voice\\Rob\\MoveTo.wss", MakeEntry(1000)));
    REQUIRE(cache.Find("voice/rob/moveto.wss") != nullptr);
    REQUIRE(cache.Contains("VOICE\\ROB\\MOVETO.WSS"));

    auto stats = cache.GetStats();
    REQUIRE(stats.entries == 1);
    REQUIRE(stats.bytes == 1000);
    REQUIRE(stats.hits == 1);
    REQUIRE(stats.inserts == 1);
}

TEST_CASE("PcmCache: miss counting and duplicate-insert rejection", "[Audio][PcmCache]")
{
    PcmCache cache;
    REQUIRE(cache.Find("absent.wss") == nullptr);
    REQUIRE(cache.GetStats().misses == 1);

    REQUIRE(cache.Insert("a.wss", MakeEntry(10)));
    REQUIRE_FALSE(cache.Insert("A.WSS", MakeEntry(10))); // first decode wins
    REQUIRE(cache.GetStats().entries == 1);
}

TEST_CASE("PcmCache: oversized entries are rejected", "[Audio][PcmCache]")
{
    PcmCache cache;
    REQUIRE_FALSE(cache.Insert("music.wss", MakeEntry(PcmCache::kMaxEntryBytes + 1)));
    REQUIRE_FALSE(cache.Insert("empty.wss", MakeEntry(0)));
    REQUIRE(cache.GetStats().entries == 0);
}

TEST_CASE("PcmCache: LRU eviction at the total-bytes cap evicts the least recently used", "[Audio][PcmCache]")
{
    PcmCache cache;
    // Fill the cache with 16 entries of 4 MB = 64 MB (exactly at cap).
    const size_t entryBytes = PcmCache::kMaxEntryBytes;
    char name[32];
    for (int i = 0; i < 16; i++)
    {
        snprintf(name, sizeof(name), "wave%02d.wss", i);
        REQUIRE(cache.Insert(name, MakeEntry(entryBytes)));
    }
    REQUIRE(cache.GetStats().entries == 16);

    // Touch wave00 so wave01 becomes the LRU victim.
    REQUIRE(cache.Find("wave00.wss") != nullptr);

    REQUIRE(cache.Insert("overflow.wss", MakeEntry(entryBytes)));
    auto stats = cache.GetStats();
    REQUIRE(stats.entries == 16);
    REQUIRE(stats.evictions == 1);
    REQUIRE(stats.bytes <= PcmCache::kMaxTotalBytes);
    REQUIRE(cache.Contains("wave00.wss"));       // recently used — survives
    REQUIRE_FALSE(cache.Contains("wave01.wss")); // LRU — evicted
    REQUIRE(cache.Contains("overflow.wss"));
}

TEST_CASE("PcmCache: Clear drops entries but keeps cumulative counters", "[Audio][PcmCache]")
{
    PcmCache cache;
    REQUIRE(cache.Insert("a.wss", MakeEntry(10)));
    REQUIRE(cache.Find("a.wss") != nullptr);
    cache.Clear();
    auto stats = cache.GetStats();
    REQUIRE(stats.entries == 0);
    REQUIRE(stats.bytes == 0);
    REQUIRE(stats.hits == 1);
    REQUIRE(stats.inserts == 1);
}
