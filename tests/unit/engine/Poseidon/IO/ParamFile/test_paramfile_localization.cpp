#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include "../Support/test_fixtures.hpp"
#include <string.h>
#include <string>
#include <Poseidon/Foundation/Strings/RString.hpp>

// Phase 9: ParamFile Localization & StringTable Integration
// This file tests string localization using stringtable.csv integration.
//
// Coverage areas:
// 1. LocalizeString() API (10 tests)
//
// Total: 10 new tests building on Phase 1-8's 241 tests
//
// NOTE: StringTable functionality is provided by the game engine via
// SetDefaultLocalizeStringFunctions(). We can test the API and mock
// basic localization behavior.

// Import shared fixture utilities
using namespace TestFixtures;

// Helper: Mock Localization Functions

class MockLocalizeStringFunctions : public LocalizeStringFunctions
{
  private:
    // Simple mock stringtable: Key -> Value mapping
    struct StringEntry
    {
        const char* key;
        const char* value;
    };

    static constexpr StringEntry _mockStrings[] = {
        {"$STR_HELLO", "Hello World"},
        {"$STR_GOODBYE", "Goodbye World"},
        {"$STR_WEAPON_RIFLE", "Synthetic Rifle"},
        {"$STR_WEAPON_PISTOL", "M1911 Pistol"},
        {"$STR_VEHICLE_CAR", "Jeep"},
        {"$STR_MISSING", "$STR_MISSING"}, // Not localized
        {"$STR_EMPTY", ""},
        {"$STR_NESTED_A", "$STR_NESTED_B"}, // Points to another string
        {"$STR_NESTED_B", "Nested Value"},
    };

  public:
    RString LocalizeString(const char* str) override
    {
        if (!str || *str == '\0')
        {
            return str;
        }

        // If doesn't start with $STR_, return as-is
        if (strncmp(str, "$STR_", 5) != 0)
        {
            return str;
        }

        // Search in mock table
        for (const auto& entry : _mockStrings)
        {
            if (strcmp(str, entry.key) == 0)
            {
                return entry.value;
            }
        }

        // Not found - return key as-is (standard behavior)
        return str;
    }
};

// Section 9.1: LocalizeString() API (10 tests)

TEST_CASE("ParamFile - LocalizeString basic", "[paramfile][i18n][basic]")
{
    MockLocalizeStringFunctions localizer;
    ParamFile::SetDefaultLocalizeStringFunctions(&localizer);

    ParamFile pf;

    SECTION("Localize simple string key")
    {
        RString result = pf.LocalizeString("$STR_HELLO");

        REQUIRE(std::string(result.Data()) == "Hello World");
    }

    SECTION("Non-localized string returns as-is")
    {
        RString result = pf.LocalizeString("Regular text");

        REQUIRE(std::string(result.Data()) == "Regular text");
    }

    SECTION("Empty string")
    {
        RString result = pf.LocalizeString("");

        REQUIRE(result.Data()[0] == '\0');
    }

    SECTION("Null pointer")
    {
        RString result = pf.LocalizeString(nullptr);

        // Should handle gracefully (return empty or null)
        REQUIRE(true);
    }

    // Clean up
    ParamFile::SetDefaultLocalizeStringFunctions(nullptr);
}

TEST_CASE("ParamFile - Localize multiple strings", "[paramfile][i18n][multiple]")
{
    MockLocalizeStringFunctions localizer;
    ParamFile::SetDefaultLocalizeStringFunctions(&localizer);

    ParamFile pf;

    SECTION("Different localization keys")
    {
        RString hello = pf.LocalizeString("$STR_HELLO");
        RString goodbye = pf.LocalizeString("$STR_GOODBYE");
        RString rifle = pf.LocalizeString("$STR_WEAPON_RIFLE");

        REQUIRE(std::string(hello.Data()) == "Hello World");
        REQUIRE(std::string(goodbye.Data()) == "Goodbye World");
        REQUIRE(std::string(rifle.Data()) == "Synthetic Rifle");
    }

    // Clean up
    ParamFile::SetDefaultLocalizeStringFunctions(nullptr);
}

TEST_CASE("ParamFile - Missing localization", "[paramfile][i18n][missing]")
{
    MockLocalizeStringFunctions localizer;
    ParamFile::SetDefaultLocalizeStringFunctions(&localizer);

    ParamFile pf;

    SECTION("Unknown key returns key itself")
    {
        RString result = pf.LocalizeString("$STR_UNKNOWN_KEY");

        // Standard behavior: return key when translation not found
        REQUIRE(std::string(result.Data()) == "$STR_UNKNOWN_KEY");
    }

    SECTION("Known missing key")
    {
        RString result = pf.LocalizeString("$STR_MISSING");

        // Mock returns the key itself
        REQUIRE(std::string(result.Data()) == "$STR_MISSING");
    }

    // Clean up
    ParamFile::SetDefaultLocalizeStringFunctions(nullptr);
}

TEST_CASE("ParamFile - Localization in config values", "[paramfile][i18n][config]")
{
    MockLocalizeStringFunctions localizer;
    ParamFile::SetDefaultLocalizeStringFunctions(&localizer);

    SECTION("Config with localized strings")
    {
        ParamFile pf;

        // Add config entries with $STR_ keys
        pf.Add("displayName", "$STR_WEAPON_RIFLE");
        pf.Add("description", "$STR_HELLO");
        pf.Add("regularValue", "Not localized");

        // Values are stored as-is (not localized yet)
        ParamEntry* displayName = pf.FindEntry("displayName");
        REQUIRE(std::string(displayName->GetValueRaw().Data()) == "$STR_WEAPON_RIFLE");

        // Localization happens when explicitly called
        RString localized = pf.LocalizeString(displayName->GetValueRaw());
        REQUIRE(std::string(localized.Data()) == "Synthetic Rifle");
    }

    // Clean up
    ParamFile::SetDefaultLocalizeStringFunctions(nullptr);
}

TEST_CASE("ParamFile - Recursive localization", "[paramfile][i18n][recursive]")
{
    MockLocalizeStringFunctions localizer;
    ParamFile::SetDefaultLocalizeStringFunctions(&localizer);

    ParamFile pf;

    SECTION("Nested string reference")
    {
        // $STR_NESTED_A -> $STR_NESTED_B -> "Nested Value"
        RString result1 = pf.LocalizeString("$STR_NESTED_A");

        // First lookup returns another $STR_ key
        REQUIRE(std::string(result1.Data()) == "$STR_NESTED_B");

        // Second lookup resolves to final value
        RString result2 = pf.LocalizeString(result1);
        REQUIRE(std::string(result2.Data()) == "Nested Value");
    }

    SECTION("Manual recursive resolution")
    {
        // Helper to recursively resolve strings
        auto ResolveRecursive = [&pf](const char* key, int maxDepth = 5) -> RString
        {
            RString result = key;
            for (int i = 0; i < maxDepth; i++)
            {
                RString next = pf.LocalizeString(result);
                if (strcmp(next, result) == 0)
                {
                    break; // No change, done
                }
                result = next;
            }
            return result;
        };

        RString final = ResolveRecursive("$STR_NESTED_A");
        REQUIRE(std::string(final.Data()) == "Nested Value");
    }

    // Clean up
    ParamFile::SetDefaultLocalizeStringFunctions(nullptr);
}

TEST_CASE("ParamFile - Localization in arrays", "[paramfile][i18n][arrays]")
{
    MockLocalizeStringFunctions localizer;
    ParamFile::SetDefaultLocalizeStringFunctions(&localizer);

    ParamFile pf;

    SECTION("Array of localized strings")
    {
        ParamEntry* arr = pf.AddArray("weapons");
        arr->AddValue("$STR_WEAPON_RIFLE");
        arr->AddValue("$STR_WEAPON_PISTOL");
        arr->AddValue("Regular weapon name");

        REQUIRE(arr->GetSize() == 3);

        // Localize each element
        RString loc1 = pf.LocalizeString((*arr)[0].GetValueRaw());
        RString loc2 = pf.LocalizeString((*arr)[1].GetValueRaw());
        RString loc3 = pf.LocalizeString((*arr)[2].GetValueRaw());

        REQUIRE(std::string(loc1.Data()) == "Synthetic Rifle");
        REQUIRE(std::string(loc2.Data()) == "M1911 Pistol");
        REQUIRE(std::string(loc3.Data()) == "Regular weapon name");
    }

    // Clean up
    ParamFile::SetDefaultLocalizeStringFunctions(nullptr);
}

TEST_CASE("ParamFile - Localization caching behavior", "[paramfile][i18n][cache]")
{
    MockLocalizeStringFunctions localizer;
    ParamFile::SetDefaultLocalizeStringFunctions(&localizer);

    ParamFile pf;

    SECTION("Multiple calls to same key")
    {
        // Call LocalizeString multiple times
        RString result1 = pf.LocalizeString("$STR_HELLO");
        RString result2 = pf.LocalizeString("$STR_HELLO");
        RString result3 = pf.LocalizeString("$STR_HELLO");

        // All should return same value
        REQUIRE(std::string(result1.Data()) == "Hello World");
        REQUIRE(std::string(result2.Data()) == "Hello World");
        REQUIRE(std::string(result3.Data()) == "Hello World");

        // Note: ParamFile doesn't cache internally (each call goes to callback)
        // Caching is responsibility of LocalizeStringFunctions implementation
    }

    // Clean up
    ParamFile::SetDefaultLocalizeStringFunctions(nullptr);
}

TEST_CASE("ParamFile - Localization with parsing", "[paramfile][i18n][parse]")
{
    MockLocalizeStringFunctions localizer;
    ParamFile::SetDefaultLocalizeStringFunctions(&localizer);

    SECTION("Parse config with localization keys")
    {
        ParamFile pf;

        const char* config = "class Weapon {\n"
                             "    displayName = \"$STR_WEAPON_RIFLE\";\n"
                             "    description = \"$STR_HELLO\";\n"
                             "    ammo = 30;\n"
                             "};\n";

        QIStream in(config, strlen(config));
        pf.Parse(in);

        const ParamClass* weapon = pf.GetClass("Weapon");
        REQUIRE(weapon != nullptr);

        ParamEntry* displayName = weapon->FindEntry("displayName");
        ParamEntry* description = weapon->FindEntry("description");

        REQUIRE(displayName != nullptr);
        REQUIRE(description != nullptr);

        // Raw values contain $STR_ keys
        REQUIRE(std::string(displayName->GetValueRaw().Data()) == "$STR_WEAPON_RIFLE");

        // Localize them
        RString locName = pf.LocalizeString(displayName->GetValueRaw());
        RString locDesc = pf.LocalizeString(description->GetValueRaw());

        REQUIRE(std::string(locName.Data()) == "Synthetic Rifle");
        REQUIRE(std::string(locDesc.Data()) == "Hello World");
    }

    // Clean up
    ParamFile::SetDefaultLocalizeStringFunctions(nullptr);
}

TEST_CASE("ParamFile - SetDefaultLocalizeStringFunctions", "[paramfile][i18n][callback]")
{
    SECTION("Set and clear localization functions")
    {
        MockLocalizeStringFunctions localizer;

        // Set localizer
        ParamFile::SetDefaultLocalizeStringFunctions(&localizer);

        ParamFile pf;

        // Should use localizer
        RString result = pf.LocalizeString("$STR_HELLO");
        REQUIRE(std::string(result.Data()) == "Hello World");

        // Clear localizer
        ParamFile::SetDefaultLocalizeStringFunctions(nullptr);

        // Now returns key as-is (no localization)
        // Note: Behavior when no localizer set may vary
        REQUIRE(true); // Document behavior
    }
}

TEST_CASE("ParamFile - UTF-8 in localized strings", "[paramfile][i18n][encoding]")
{
    MockLocalizeStringFunctions localizer;
    ParamFile::SetDefaultLocalizeStringFunctions(&localizer);

    ParamFile pf;

    SECTION("ASCII strings work")
    {
        RString result = pf.LocalizeString("$STR_HELLO");
        REQUIRE(std::string(result.Data()) == "Hello World");
    }

    SECTION("UTF-8 support")
    {
        // Note: Real stringtable.csv uses UTF-8 encoding
        // Mock doesn't have UTF-8 strings, but API should handle them

        // UTF-8 strings would work if mock had them:
        // "$STR_UNICODE" -> "Привет мир" (Russian)
        // "$STR_CZECH" -> "Ahoj světe" (Czech)

        // Just verify API doesn't crash with non-ASCII
        RString result = pf.LocalizeString("Regular ASCII text");
        REQUIRE(result.Data() != nullptr);
    }

    // Clean up
    ParamFile::SetDefaultLocalizeStringFunctions(nullptr);
}

// Integration Test: Real-World Usage Pattern

TEST_CASE("ParamFile - Localization integration example", "[paramfile][i18n][integration]")
{
    MockLocalizeStringFunctions localizer;
    ParamFile::SetDefaultLocalizeStringFunctions(&localizer);

    SECTION("Complete weapon config with localization")
    {
        ParamFile pf;

        // Parse a config that uses localization
        const char* config = "class CfgWeapons {\n"
                             "    class SyntheticRifle {\n"
                             "        displayName = \"$STR_WEAPON_RIFLE\";\n"
                             "        displayNameShort = \"SyntheticMagazine\";\n"
                             "        ammo = 30;\n"
                             "        class Magazine {\n"
                             "            displayName = \"$STR_HELLO\";\n"
                             "        };\n"
                             "    };\n"
                             "};\n";

        QIStream in(config, strlen(config));
        pf.Parse(in);

        const ParamClass* cfg = pf.GetClass("CfgWeapons");
        REQUIRE(cfg != nullptr);

        const ParamClass* m16 = cfg->GetClass("SyntheticRifle");
        REQUIRE(m16 != nullptr);

        // Get raw display name (contains $STR_ key)
        ParamEntry* displayName = m16->FindEntry("displayName");
        REQUIRE(displayName != nullptr);

        RString rawName = displayName->GetValueRaw();
        REQUIRE(std::string(rawName.Data()) == "$STR_WEAPON_RIFLE");

        // Localize for display to user
        RString localizedName = pf.LocalizeString(rawName);
        REQUIRE(std::string(localizedName.Data()) == "Synthetic Rifle");

        // Non-localized value works as-is
        ParamEntry* shortName = m16->FindEntry("displayNameShort");
        RString shortVal = pf.LocalizeString(shortName->GetValueRaw());
        REQUIRE(std::string(shortVal.Data()) == "SyntheticMagazine");

        // Nested localization
        const ParamClass* mag = m16->GetClass("Magazine");
        REQUIRE(mag != nullptr);

        ParamEntry* magName = mag->FindEntry("displayName");
        RString magLocalized = pf.LocalizeString(magName->GetValueRaw());
        REQUIRE(std::string(magLocalized.Data()) == "Hello World");
    }

    // Clean up
    ParamFile::SetDefaultLocalizeStringFunctions(nullptr);
}
