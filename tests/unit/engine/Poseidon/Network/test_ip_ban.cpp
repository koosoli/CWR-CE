// IPv4 ban parsing/formatting + banMode. The server stores IP
// bans in ipban.txt (dotted quads) and matches a connecting client's IP against
// them, so the parse must accept exactly well-formed quads (with the trailing
// newline ipban.txt lines carry) and reject everything else, round-trip with the
// formatter, and banMode must default to Both.
#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Network/IpBan.hpp>
#include <cstdint>
#include <string>

using Poseidon::BanMode;
using Poseidon::FormatIPv4;
using Poseidon::ParseBanMode;
using Poseidon::ParseIPv4;

TEST_CASE("ParseIPv4 - valid dotted quads", "[network][ipban]")
{
    uint32_t ip = 0;
    REQUIRE(ParseIPv4("1.2.3.4", ip));
    REQUIRE(ip == ((1u << 24) | (2u << 16) | (3u << 8) | 4u));
    REQUIRE(std::string(static_cast<const char*>(FormatIPv4(ip))) == "1.2.3.4");

    REQUIRE(ParseIPv4("0.0.0.0", ip));
    REQUIRE(ip == 0u);
    REQUIRE(ParseIPv4("255.255.255.255", ip));
    REQUIRE(ip == 0xFFFFFFFFu);

    // ipban.txt lines carry a trailing newline; surrounding whitespace is tolerated.
    REQUIRE(ParseIPv4("192.168.1.50\n", ip));
    REQUIRE(ip == ((192u << 24) | (168u << 16) | (1u << 8) | 50u));
    REQUIRE(ParseIPv4("  10.0.0.1  ", ip));
}

TEST_CASE("ParseIPv4 - rejects malformed input", "[network][ipban]")
{
    uint32_t ip = 0;
    REQUIRE_FALSE(ParseIPv4(nullptr, ip));
    REQUIRE_FALSE(ParseIPv4("", ip));
    REQUIRE_FALSE(ParseIPv4("1.2.3", ip));     // too few octets
    REQUIRE_FALSE(ParseIPv4("1.2.3.4.5", ip)); // too many
    REQUIRE_FALSE(ParseIPv4("256.0.0.1", ip)); // octet out of range
    REQUIRE_FALSE(ParseIPv4("1.2.3.999", ip));
    REQUIRE_FALSE(ParseIPv4("a.b.c.d", ip));  // non-numeric
    REQUIRE_FALSE(ParseIPv4("1.2.3.4x", ip)); // trailing junk
    REQUIRE_FALSE(ParseIPv4("1..3.4", ip));   // empty octet
}

TEST_CASE("FormatIPv4 round-trips ParseIPv4", "[network][ipban]")
{
    for (const char* s : {"8.8.8.8", "192.168.0.1", "172.16.254.1", "127.0.0.1"})
    {
        uint32_t ip = 0;
        REQUIRE(ParseIPv4(s, ip));
        REQUIRE(std::string(static_cast<const char*>(FormatIPv4(ip))) == s);
        uint32_t ip2 = 0;
        REQUIRE(ParseIPv4(static_cast<const char*>(FormatIPv4(ip)), ip2));
        REQUIRE(ip == ip2);
    }
}

TEST_CASE("ParseBanMode - id/ip/both, default Both", "[network][ipban]")
{
    REQUIRE(ParseBanMode("id") == BanMode::Id);
    REQUIRE(ParseBanMode("ip") == BanMode::Ip);
    REQUIRE(ParseBanMode("both") == BanMode::Both);
    REQUIRE(ParseBanMode("ID") == BanMode::Id); // case-insensitive
    REQUIRE(ParseBanMode("IP") == BanMode::Ip);
    REQUIRE(ParseBanMode(nullptr) == BanMode::Both);
    REQUIRE(ParseBanMode("") == BanMode::Both);
    REQUIRE(ParseBanMode("nonsense") == BanMode::Both);
}
