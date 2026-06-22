// Server ban-list mutation behind #ban / #unban.  #ban appends a
// player id (or IP) to the dynamic ban list and rewrites ban.txt / ipban.txt;
// #unban removes it by value.  These pin the save/load file-format round-trip
// and the remove-by-value semantics Unban() relies on — FindArray::Delete(value)
// must remove the matching entry, not the entry at that index.
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Network/NetworkServerCommon.hpp>
#include <Poseidon/Network/IpBan.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include "../Support/test_fixtures.hpp"

using namespace Poseidon;

TEST_CASE("ban id list: save/load round-trip + unban removal", "[network][ban]")
{
    const char* path = TestFixtures::GetTempFilePath("test_ban_ids.txt");

    FindArray<__int64> list;
    list.AddUnique(static_cast<__int64>(4138271649283LL)); // #ban player A
    list.AddUnique(static_cast<__int64>(777));             // #ban player B
    SaveBanList(path, list);

    FindArray<__int64> loaded;
    LoadBanList(path, loaded);
    REQUIRE(loaded.Size() == 2);
    CHECK(loaded.Find(static_cast<__int64>(4138271649283LL)) >= 0);
    CHECK(loaded.Find(static_cast<__int64>(777)) >= 0);

    // #unban A: remove by value, then rewrite. The other ban must survive.
    REQUIRE(loaded.Delete(static_cast<__int64>(4138271649283LL)));
    SaveBanList(path, loaded);

    FindArray<__int64> after;
    LoadBanList(path, after);
    CHECK(after.Size() == 1);
    CHECK(after.Find(static_cast<__int64>(4138271649283LL)) < 0);
    CHECK(after.Find(static_cast<__int64>(777)) >= 0);

    TestFixtures::CleanupTempFile(path);
}

TEST_CASE("ban ip list: save/load round-trip + unban removal", "[network][ban]")
{
    const char* path = TestFixtures::GetTempFilePath("test_ban_ips.txt");

    uint32_t a = 0, b = 0;
    REQUIRE(ParseIPv4("203.0.113.5", a));
    REQUIRE(ParseIPv4("198.51.100.9", b));

    FindArray<uint32_t> list;
    list.AddUnique(a);
    list.AddUnique(b);
    SaveIpBanList(path, list);

    FindArray<uint32_t> loaded;
    LoadIpBanList(path, loaded);
    REQUIRE(loaded.Size() == 2);

    REQUIRE(loaded.Delete(a)); // #unban by IP
    SaveIpBanList(path, loaded);

    FindArray<uint32_t> after;
    LoadIpBanList(path, after);
    CHECK(after.Size() == 1);
    CHECK(after.Find(a) < 0);
    CHECK(after.Find(b) >= 0);

    TestFixtures::CleanupTempFile(path);
}

TEST_CASE("unban arg discrimination: dotted quad = IP, decimal = id", "[network][ban]")
{
    // Unban() routes a dotted quad to the IP list and a bare decimal to the id
    // list; a player id must NOT parse as an IP.
    uint32_t ip = 0;
    CHECK(ParseIPv4("192.168.1.50", ip));
    CHECK_FALSE(ParseIPv4("4138271649283", ip));
}
