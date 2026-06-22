#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Dev/Debug/DebugCommands.hpp>

#include <string>
#include <string_view>
#include <vector>

using namespace Poseidon::Dev;

namespace
{
// Tracks invocations for the test commands so we can observe what the
// registry dispatched.  Reset at the start of every TEST_CASE.
struct Probe
{
    int dummyInvocations = 0;
    int gatedInvocations = 0;
    bool gatedAvailable = true;
    std::string lastArgs;
};
Probe g_probe;

bool DummyAvailable()
{
    return true;
}
void DummyInvoke(std::string_view args, std::string& out)
{
    g_probe.dummyInvocations++;
    g_probe.lastArgs = std::string(args);
    out = "dummy ok";
}

bool GatedAvailable()
{
    return g_probe.gatedAvailable;
}
void GatedInvoke(std::string_view /*args*/, std::string& out)
{
    g_probe.gatedInvocations++;
    out = "gated ok";
}
} // namespace

TEST_CASE("DebugCommands: register + find", "[debug][commands]")
{
    g_probe = Probe{};
    DebugCommands::Register({"dctest_dummy", "test command", DummyAvailable, DummyInvoke});
    REQUIRE(DebugCommands::Find("dctest_dummy") != nullptr);
    REQUIRE(DebugCommands::Find("dctest_unknown") == nullptr);
}

TEST_CASE("DebugCommands: Run dispatches to invoke and captures args", "[debug][commands]")
{
    g_probe = Probe{};
    DebugCommands::Register({"dctest_dummy", "test command", DummyAvailable, DummyInvoke});
    std::string out;
    REQUIRE(DebugCommands::Run("dctest_dummy hello world", out));
    REQUIRE(g_probe.dummyInvocations == 1);
    REQUIRE(g_probe.lastArgs == "hello world");
    REQUIRE(out == "dummy ok");
}

TEST_CASE("DebugCommands: Run trims leading whitespace and empty args", "[debug][commands]")
{
    g_probe = Probe{};
    DebugCommands::Register({"dctest_dummy", "test command", DummyAvailable, DummyInvoke});
    std::string out;
    REQUIRE(DebugCommands::Run("   dctest_dummy", out));
    REQUIRE(g_probe.dummyInvocations == 1);
    REQUIRE(g_probe.lastArgs.empty());
}

TEST_CASE("DebugCommands: Run on unknown command reports it", "[debug][commands]")
{
    std::string out;
    REQUIRE_FALSE(DebugCommands::Run("dctest_does_not_exist arg", out));
    REQUIRE(out.find("unknown command") != std::string::npos);
}

TEST_CASE("DebugCommands: unavailable command is gated without invoking", "[debug][commands]")
{
    g_probe = Probe{};
    g_probe.gatedAvailable = false;
    DebugCommands::Register({"dctest_gated", "gated test", GatedAvailable, GatedInvoke});
    std::string out;
    // Run returns true (command was found) but invoke must not fire.
    REQUIRE(DebugCommands::Run("dctest_gated", out));
    REQUIRE(g_probe.gatedInvocations == 0);
    REQUIRE(out.find("unavailable") != std::string::npos);
}

TEST_CASE("DebugCommands: duplicate registration is a no-op", "[debug][commands]")
{
    g_probe = Probe{};
    DebugCommands::Register({"dctest_dup", "first", DummyAvailable, DummyInvoke});
    DebugCommands::Register({"dctest_dup", "second", DummyAvailable, DummyInvoke});
    // Only one entry — All() must contain exactly one match.
    int count = 0;
    for (const auto& c : DebugCommands::All())
        if (std::string_view(c.name) == "dctest_dup")
            count++;
    REQUIRE(count == 1);
}
