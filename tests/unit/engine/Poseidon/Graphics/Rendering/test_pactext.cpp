#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Graphics/Rendering/Font/Pactext.hpp>
#include <Poseidon/Foundation/Logging/Logging.hpp>

TEST_CASE("pactext.hpp compiles", "[rendering][font]")
{
    SUCCEED("header included successfully");
}

// SelectTextureSourceFactory is the texture-load entry the briefing equipment
// screen hits per gear slot.  An empty optional slot resolves to a directory-
// only path ("dtaext\equip\") with no filename — the original engine skipped it
// silently.  Without the guard the selector logs an "Unrecognized texture type"
// ERROR for every empty slot, which strict mode turns fatal (exit 3) on the
// briefing / credits screens.
//
// Broken-state delta: without the empty-filename guard, the "dtaext\equip\"
// case bumps GetErrorCount() to 1 (the LOG_ERROR fires); with it, count stays 0.
TEST_CASE("SelectTextureSourceFactory: empty-filename path is no-texture, not an error", "[rendering][font]")
{
    using LS = Poseidon::Foundation::LoggingSystem;
    LS logSys;
    logSys.Initialize("trace"); // attaches the ErrorCountingSink to category loggers
    LS::SetStrictMode(false);   // count errors without latching the strict trip

    SECTION("directory-only path (empty equipment slot) is silent")
    {
        LS::ResetErrorCount();
        auto* f = Poseidon::SelectTextureSourceFactory("dtaext\\equip\\");
        CHECK(f == nullptr);
        CHECK(LS::GetErrorCount() == 0); // the fix: no ERROR for a fileless path
    }

    SECTION("a real filename with an unknown extension still errors")
    {
        LS::ResetErrorCount();
        auto* f = Poseidon::SelectTextureSourceFactory("equip\\w\\w_bad.xyz");
        CHECK(f == nullptr);
        CHECK(LS::GetErrorCount() >= 1); // genuinely unrecognized → still reported
    }

    SECTION("a .paa name resolves to the PAC factory")
    {
        LS::ResetErrorCount();
        auto* f = Poseidon::SelectTextureSourceFactory("equip\\w\\w_m16.paa");
        CHECK(f != nullptr);
        CHECK(LS::GetErrorCount() == 0);
    }

    SECTION("empty and null names are silent")
    {
        LS::ResetErrorCount();
        CHECK(Poseidon::SelectTextureSourceFactory("") == nullptr);
        CHECK(Poseidon::SelectTextureSourceFactory(nullptr) == nullptr);
        CHECK(LS::GetErrorCount() == 0);
    }

    LS::ResetErrorCount();
}
