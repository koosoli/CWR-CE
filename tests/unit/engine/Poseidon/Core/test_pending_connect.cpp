// PendingConnect carries a server target (address/port/password) across the
// mod-apply re-mount so the fresh boot can finish the join.
// It must be a one-shot: arm once, consume once, and never reconnect by accident on
// a later boot.

#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Core/PendingConnect.hpp>

using Poseidon::PendingConnect;

TEST_CASE("PendingConnect - default is disarmed", "[mods][mpjoin]")
{
    const PendingConnect pc;
    REQUIRE_FALSE(pc.IsArmed());
    REQUIRE(pc.Address().empty());
    REQUIRE(pc.Port() == 0);
}

TEST_CASE("PendingConnect - Arm stashes the target", "[mods][mpjoin]")
{
    PendingConnect pc;
    pc.Arm("192.168.0.12", 2302, "secret");
    REQUIRE(pc.IsArmed());
    REQUIRE(pc.Address() == "192.168.0.12");
    REQUIRE(pc.Port() == 2302);
    REQUIRE(pc.Password() == "secret");
}

TEST_CASE("PendingConnect - Disarm clears it (no accidental reconnect)", "[mods][mpjoin]")
{
    PendingConnect pc;
    pc.Arm("10.0.0.1", 2310, "pw");
    pc.Disarm();
    REQUIRE_FALSE(pc.IsArmed()); // teeth: a consumed/cancelled target must not linger
    REQUIRE(pc.Address().empty());
    REQUIRE(pc.Port() == 0);
    REQUIRE(pc.Password().empty());
}

TEST_CASE("PendingConnect - global singleton is shared", "[mods][mpjoin]")
{
    Poseidon::GPendingConnect().Disarm();
    REQUIRE_FALSE(Poseidon::GPendingConnect().IsArmed());
    Poseidon::GPendingConnect().Arm("127.0.0.1", 2302, "");
    REQUIRE(Poseidon::GPendingConnect().IsArmed());
    REQUIRE(Poseidon::GPendingConnect().Address() == "127.0.0.1");
    Poseidon::GPendingConnect().Disarm(); // leave clean for any later test
}
