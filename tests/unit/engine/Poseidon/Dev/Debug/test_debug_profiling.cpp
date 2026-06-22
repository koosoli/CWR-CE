#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Dev/Debug/DebugTrap.hpp>

using namespace Poseidon::Dev;
#include <Poseidon/Dev/Debug/DebugWin.hpp>
#include <Poseidon/Dev/Diag/VtuneProf.hpp>

TEST_CASE("debugTrap: Debugger compile-only", "[debug][debugTrap]")
{
    // Tier 3: verify header compiles and Debugger class is usable
    (void)GDebugger.IsDebugger();
    SUCCEED();
}

TEST_CASE("debugWin: header compile-only", "[debug][debugWin]")
{
    SUCCEED("debugWin.hpp compiles");
}

TEST_CASE("vtuneProf: header compile-only", "[debug][vtuneProf]")
{
    SUCCEED("vtuneProf.hpp compiles");
}
