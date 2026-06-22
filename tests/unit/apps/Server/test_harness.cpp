#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Dev/Harness/HarnessServer.hpp>

using Poseidon::Dev::HarnessServer;

TEST_CASE("HarnessServer can auto-assign a localhost port", "[simulate][harness]")
{
    HarnessServer server;
    REQUIRE(server.Start(0));
    REQUIRE(server.IsRunning());
    REQUIRE(server.GetPort() > 0);
    server.Stop();
}
