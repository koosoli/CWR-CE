#include <Poseidon/Foundation/PoseidonPCH.hpp>
#include <catch2/catch_test_macros.hpp>

#include <Poseidon/IO/ParamFile/InitLibraryElement.hpp>
#include <Poseidon/IO/ParamFile/LocalizedString.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include <Poseidon/UI/Locale/Stringtable/Stringtable.hpp>

#include <cstring>
#include <filesystem>
#include <string>

// In Czech, editor/map object names (House, Church, Fountain, ...) stayed in English.
// Root cause: EntityAIType's plain RString _displayName snapshotted the CfgVehicles
// displayName ($STR_DN_* token) at config-load (boot language), so after
// Poseidon::SetLanguage the cached English text was never refreshed.
//
// Fix: EntityAIType::_displayName is now a LocalizedString bound to the config node
// (VehicleAI::Load: `_displayName.Bind(par >> "displayName")`), and GetDisplayName()
// returns it, so it re-resolves against the live language. This test exercises that
// exact bind-then-read-as-RStringB path with the building-name tokens from the bug.
//
// Broken-state delta: snapshot the value at load (the old RString member) and the
// name reads back "Church"/"House" after SetLanguage("Czech") instead of
// "Kostel"/"Dum". ASCII Czech keeps the assertion an exact match (no codepage
// transcode in play). The LocalizedString primitive itself is unit-tested in
// tests/unit/engine/Poseidon/IO/ParamFile/test_localized_string.cpp.

using namespace Poseidon;

namespace
{
void LoadObjectNameStringtable()
{
    Poseidon::InitParamFileStringtable();

    Poseidon::ClearStringtable();
    GLanguage = "English";
    const std::string csv =
        (std::filesystem::path(TESTS_ROOT_DIR) / "fixtures" / "stringtable" / "object_names_lang.csv").string();
    Poseidon::LoadStringtable("global", csv.c_str(), 0, true);

    // Guard the routing itself so a later mismatch is the caching regression, not a load failure.
    REQUIRE(std::string(Poseidon::LocalizeString("STR_DN_CHURCH").Data()) == "Church");
    REQUIRE(Poseidon::SetLanguage("Czech"));
    REQUIRE(std::string(Poseidon::LocalizeString("STR_DN_CHURCH").Data()) == "Kostel");
    REQUIRE(Poseidon::SetLanguage("English"));
}

ParamFile ParseConfig(const char* text)
{
    ParamFile pf;
    QIStream in(text, static_cast<int>(strlen(text)));
    pf.Parse(in);
    return pf;
}

// Mirrors EntityAIType: a LocalizedString displayName bound to the config node,
// read back through the same RStringB conversion GetDisplayName() uses.
RStringB ReadName(const LocalizedString& displayName)
{
    return displayName;
}
} // namespace

TEST_CASE("Building/object display name follows runtime language switch", "[ai][localization][switch]")
{
    LoadObjectNameStringtable();

    ParamFile pf = ParseConfig("class Church\n"
                               "{\n"
                               "    displayName = \"$STR_DN_CHURCH\";\n"
                               "};\n"
                               "class House\n"
                               "{\n"
                               "    displayName = \"$STR_DN_HOUSE\";\n"
                               "};\n");
    REQUIRE(pf.FindEntry("Church") != nullptr);

    LocalizedString churchName;
    churchName.Bind(pf >> "Church" >> "displayName");
    LocalizedString houseName;
    houseName.Bind(pf >> "House" >> "displayName");

    // Resolved under English.
    REQUIRE(std::string(ReadName(churchName).Data()) == "Church");
    REQUIRE(std::string(ReadName(houseName).Data()) == "House");

    // After a runtime language switch the object names must follow.
    REQUIRE(Poseidon::SetLanguage("Czech"));
    REQUIRE(std::string(ReadName(churchName).Data()) == "Kostel");
    REQUIRE(std::string(ReadName(houseName).Data()) == "Dum");

    // ...and back, proving it tracks the live language rather than latching once.
    REQUIRE(Poseidon::SetLanguage("English"));
    REQUIRE(std::string(ReadName(churchName).Data()) == "Church");
}
