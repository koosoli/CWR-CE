#include <catch2/catch_test_macros.hpp>
#include "../Support/test_fixtures.hpp"

#include <Poseidon/IO/ParamFile/InitLibraryElement.hpp>
#include <Poseidon/IO/ParamFile/LocalizedString.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include <Poseidon/UI/Locale/Stringtable/Stringtable.hpp>

#include <cstring>
#include <string>

// Isolation tests for LocalizedString — the engine primitive that resolves a
// localized config value on demand and re-resolves it when the active language
// changes, instead of snapshotting it once at load time (the bug behind GitLab
// #25). These tests exercise the type directly against a two-language
// (English/Czech) stringtable, with no weapon/UI involvement; the end-to-end
// weapon-HUD regression lives in
// tests/unit/.../World/Entities/Weapons/test_weapon_displayname_lang_switch.cpp.

using namespace Poseidon;
using namespace TestFixtures;

namespace
{
// Two-language stringtable: STR_DN_BURST = "Burst"/"Davka" (ASCII Czech so the
// assertions are exact-match and independent of codepage transcoding).
void LoadLocaleFixture()
{
    Poseidon::InitParamFileStringtable(); // wire $STR_ -> stringtable for ParamFile reads
    Poseidon::ClearStringtable();
    GLanguage = "English";
    Poseidon::LoadStringtable("global", GetTestFixturePath("weapon_modes_lang.csv"), 0, true);
    REQUIRE(std::string(Poseidon::LocalizeString("STR_DN_BURST").Data()) == "Burst");
}

ParamFile ParseConfig(const char* text)
{
    ParamFile pf;
    QIStream in(text, static_cast<int>(strlen(text)));
    pf.Parse(in);
    return pf;
}
} // namespace

TEST_CASE("LocalizedString re-resolves a $STR value after a language switch", "[paramfile][localized][switch]")
{
    LoadLocaleFixture();

    ParamFile pf = ParseConfig("class T { loc = \"$STR_DN_BURST\"; };\n");

    LocalizedString loc;
    loc.Bind(pf >> "T" >> "loc");
    REQUIRE(loc.IsBound());

    // Resolves against the current language on demand.
    REQUIRE(std::string(loc.CStr()) == "Burst");

    REQUIRE(Poseidon::SetLanguage("Czech"));
    REQUIRE(std::string(loc.CStr()) == "Davka");

    REQUIRE(Poseidon::SetLanguage("English"));
    REQUIRE(std::string(loc.CStr()) == "Burst");

    // operator RStringB() resolves the same way.
    REQUIRE(Poseidon::SetLanguage("Czech"));
    const RStringB asRString = loc;
    REQUIRE(std::string(asRString.Data()) == "Davka");
}

TEST_CASE("LocalizedString returns a stable value across repeated reads and same-language no-ops",
          "[paramfile][localized]")
{
    LoadLocaleFixture();

    ParamFile pf = ParseConfig("class T { loc = \"$STR_DN_BURST\"; };\n");

    LocalizedString loc;
    loc.Bind(pf >> "T" >> "loc");

    REQUIRE(std::string(loc.CStr()) == "Burst");
    REQUIRE(std::string(loc.CStr()) == "Burst"); // cached read, same value

    // SetLanguage to the current language is a no-op and must not disturb the value.
    REQUIRE(Poseidon::SetLanguage("English"));
    REQUIRE(std::string(loc.CStr()) == "Burst");
}

TEST_CASE("LocalizedString leaves a non-$STR literal untouched across language switches", "[paramfile][localized]")
{
    LoadLocaleFixture();

    ParamFile pf = ParseConfig("class T { lit = \"PlainLiteral\"; };\n");

    LocalizedString lit;
    lit.Bind(pf >> "T" >> "lit");

    REQUIRE(std::string(lit.CStr()) == "PlainLiteral");
    REQUIRE(Poseidon::SetLanguage("Czech"));
    REQUIRE(std::string(lit.CStr()) == "PlainLiteral");
}

TEST_CASE("LocalizedString decodes legacy Central European config literals", "[paramfile][localized][encoding]")
{
    LoadLocaleFixture();

    ParamFile pf = ParseConfig("class T { name = \"\xC8"
                               "MOD\"; title = \"P\xF8"
                               "iklad \xE8"
                               "as\"; utf8 = \"\xC4\x8C"
                               "MOD\"; };\n");

    LocalizedString name;
    name.Bind(pf >> "T" >> "name");
    REQUIRE(std::string(name.CStr()) == "\xC4\x8C"
                                        "MOD");

    LocalizedString title;
    title.Bind(pf >> "T" >> "title");
    REQUIRE(std::string(title.CStr()) == "P\xC5\x99"
                                         "iklad \xC4\x8D"
                                         "as");

    LocalizedString utf8;
    utf8.Bind(pf >> "T" >> "utf8");
    REQUIRE(std::string(utf8.CStr()) == "\xC4\x8C"
                                        "MOD");

    REQUIRE(std::string((pf >> "T" >> "name").GetValue().Data()) == "\xC8"
                                                                    "MOD");
}

TEST_CASE("LocalizedString is empty when unbound or cleared, and follows a rebind", "[paramfile][localized]")
{
    LoadLocaleFixture();

    ParamFile pf = ParseConfig("class T { loc = \"$STR_DN_BURST\"; lit = \"PlainLiteral\"; };\n");

    SECTION("default-constructed is unbound and empty")
    {
        LocalizedString ls;
        REQUIRE_FALSE(ls.IsBound());
        REQUIRE(std::string(ls.CStr()).empty());
    }

    SECTION("Clear() unbinds and empties")
    {
        LocalizedString ls;
        ls.Bind(pf >> "T" >> "loc");
        REQUIRE(std::string(ls.CStr()) == "Burst");
        ls.Clear();
        REQUIRE_FALSE(ls.IsBound());
        REQUIRE(std::string(ls.CStr()).empty());
    }

    SECTION("rebinding switches the resolved source")
    {
        LocalizedString ls;
        ls.Bind(pf >> "T" >> "loc");
        REQUIRE(std::string(ls.CStr()) == "Burst");
        ls.Bind(pf >> "T" >> "lit");
        REQUIRE(std::string(ls.CStr()) == "PlainLiteral");
    }
}
