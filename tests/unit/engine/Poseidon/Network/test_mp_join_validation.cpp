// The MP-join download flow relies on a specific server
// handshake order: the dedicated server checks the client's MOD set BEFORE the
// password. A wrong mod set returns CRVersion (shared with version mismatch) and the
// password is never reached — which is exactly why a wrong password can't be caught
// before the mods are correct, so the join UI treats the password as an up-front
// gate, not a pre-download validation. This test locks that order in place.

#include <Poseidon/Foundation/PoseidonPCH.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <cstring>

#include <Poseidon/Network/NetTransportPlayerValidation.hpp>
#include <Poseidon/Network/NetTransportProtocol.hpp>

using namespace Poseidon;

namespace
{
void SetField(char* dst, std::size_t n, const char* s)
{
    std::memset(dst, 0, n);
    std::snprintf(dst, n, "%s", s);
}
} // namespace

TEST_CASE("MP join - server validates mods before password", "[mods][mpjoin][network]")
{
    SessionPacket session{};
    session.actualVersion = 100;
    session.requiredVersion = 100;
    session.equalModRequired = 1;
    SetField(session.mod, sizeof(session.mod), "@csla");

    CreatePlayerPacket req{};
    req.actualVersion = 100;
    req.requiredVersion = 100;

    const char* serverPw = "secret";

    SECTION("mod mismatch → CRVersion even with a wrong password (mods are checked first)")
    {
        SetField(req.mod, sizeof(req.mod), "@wrong");
        SetField(req.password, sizeof(req.password), "definitely-wrong");
        REQUIRE(ValidateNetTransportCreatePlayerRequest(session, serverPw, req) == CRVersion);
    }

    SECTION("mods match (case-insensitively) but wrong password → CRPassword")
    {
        SetField(req.mod, sizeof(req.mod), "@CSLA"); // stricmp in the validator
        SetField(req.password, sizeof(req.password), "nope");
        REQUIRE(ValidateNetTransportCreatePlayerRequest(session, serverPw, req) == CRPassword);
    }

    SECTION("mods and password both match → CROK")
    {
        SetField(req.mod, sizeof(req.mod), "@csla");
        SetField(req.password, sizeof(req.password), "secret");
        REQUIRE(ValidateNetTransportCreatePlayerRequest(session, serverPw, req) == CROK);
    }

    SECTION("equalModRequired=0 → mods not enforced; only the password gates entry")
    {
        session.equalModRequired = 0;
        SetField(req.mod, sizeof(req.mod), "@totally-different");
        SetField(req.password, sizeof(req.password), "secret");
        REQUIRE(ValidateNetTransportCreatePlayerRequest(session, serverPw, req) == CROK);
    }
}

// The version tag (build identity, e.g. "rc1" or the git sha; "-dev" appended for
// --dev clients) must match unconditionally — independent of equalModRequired. This
// blocks a --dev client from joining a release server and vice-versa. With matching
// versions, mods, and password but versionTag "rc1" vs "rc2",
// ValidateNetTransportCreatePlayerRequest returns CRVersion, not CROK.
TEST_CASE("MP join - server rejects mismatched version tag", "[version][mpjoin][network]")
{
    SessionPacket session{};
    session.actualVersion = 100;
    session.requiredVersion = 100;
    session.equalModRequired = 0; // mods not enforced — isolates the version-tag gate
    SetField(session.mod, sizeof(session.mod), "@csla");
    SetField(session.versionTag, sizeof(session.versionTag), "rc1");

    CreatePlayerPacket req{};
    req.actualVersion = 100;
    req.requiredVersion = 100;
    SetField(req.mod, sizeof(req.mod), "@csla");

    const char* serverPw = "";
    SetField(req.password, sizeof(req.password), "");

    SECTION("identical version tag → CROK")
    {
        SetField(req.versionTag, sizeof(req.versionTag), "rc1");
        REQUIRE(ValidateNetTransportCreatePlayerRequest(session, serverPw, req) == CROK);
    }

    SECTION("differing version tag → CRVersion even when versions, mods, and password match")
    {
        SetField(req.versionTag, sizeof(req.versionTag), "rc2");
        REQUIRE(ValidateNetTransportCreatePlayerRequest(session, serverPw, req) == CRVersion);
    }

    SECTION("a --dev client (tag suffixed '-dev') cannot join a release server")
    {
        SetField(session.versionTag, sizeof(session.versionTag), "rc1");
        SetField(req.versionTag, sizeof(req.versionTag), "rc1-dev");
        REQUIRE(ValidateNetTransportCreatePlayerRequest(session, serverPw, req) == CRVersion);
    }

    SECTION("version-tag match is case-insensitive (stricmp), like the mod compare")
    {
        SetField(session.versionTag, sizeof(session.versionTag), "RC1");
        SetField(req.versionTag, sizeof(req.versionTag), "rc1");
        REQUIRE(ValidateNetTransportCreatePlayerRequest(session, serverPw, req) == CROK);
    }
}
