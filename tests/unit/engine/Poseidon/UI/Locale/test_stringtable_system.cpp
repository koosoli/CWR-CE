// StringtableSystem — opt-in i18n gate.  ParseStringtable flips the
// global flag on successful CSV load; apps that ship no stringtable
// leave the gate false and Localize(int) returns "" silently.

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/UI/Locale/StringtableSystem.hpp>
#include <Poseidon/UI/Locale/Stringtable/Stringtable.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

using Poseidon::RegisterString;

using Poseidon::StringtableSystem;

using Poseidon::LocalizeString;

namespace Poseidon
{
extern bool g_stringtableSystemAvailable;
}
using Poseidon::g_stringtableSystemAvailable;

TEST_CASE("StringtableSystem starts unavailable", "[stringtable][system]")
{
    StringtableSystem::Instance().Shutdown();
    CHECK_FALSE(StringtableSystem::Instance().IsAvailable());
    CHECK_FALSE(g_stringtableSystemAvailable);
}

TEST_CASE("LocalizeString(int) returns empty before any stringtable loads", "[stringtable][system]")
{
    StringtableSystem::Instance().Shutdown();

    // Out-of-range id with the flag off must NOT log an error; behavior
    // contract is just an empty string returned.
    REQUIRE(LocalizeString(-1) == RString(""));
    REQUIRE(LocalizeString(99999) == RString(""));
}

TEST_CASE("StringtableSystem::Shutdown clears the gate", "[stringtable][system]")
{
    g_stringtableSystemAvailable = true;
    CHECK(StringtableSystem::Instance().IsAvailable());

    StringtableSystem::Instance().Shutdown();
    CHECK_FALSE(StringtableSystem::Instance().IsAvailable());
    CHECK_FALSE(g_stringtableSystemAvailable);

    StringtableSystem::Instance().Shutdown(); // second call no-op
    CHECK_FALSE(g_stringtableSystemAvailable);
}

TEST_CASE("StringtableSystem::IsAvailable mirrors the global flag", "[stringtable][system]")
{
    StringtableSystem::Instance().Shutdown();
    CHECK_FALSE(StringtableSystem::Instance().IsAvailable());

    g_stringtableSystemAvailable = true;
    CHECK(StringtableSystem::Instance().IsAvailable());

    g_stringtableSystemAvailable = false;
    CHECK_FALSE(StringtableSystem::Instance().IsAvailable());
}

TEST_CASE("RegisterString returns -1 when key is absent (gate independent)", "[stringtable][system]")
{
    // RegisterString itself is gate-independent — it always warns +
    // returns -1 for missing keys.  The actual subsystem gate lives at
    // INIT_MODULE(StringtableExt) (see engine/poseidon/Locale/
    // stringtableExt.cpp): when g_stringtableSystemAvailable is false
    // the eager STRING() registration loop doesn't run at all, so
    // RegisterString never gets called for IDS_*.
    CHECK(RegisterString(RString("STR_NEVER_REGISTERED_ANYWHERE")) == -1);
}
