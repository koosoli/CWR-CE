#include <catch2/catch_test_macros.hpp>

#include <Poseidon/UI/Locale/SupportedLanguages.hpp>
#include <string>

TEST_CASE("SupportedLanguages: lookup is case-insensitive", "[config][language]")
{
    CHECK(CfgLib::FindSupportedLanguageIndex("English") == 0);
    CHECK(CfgLib::FindSupportedLanguageIndex("czech") == 5);
    CHECK(CfgLib::FindSupportedLanguageIndex("RUSSIAN") == 7);
}

TEST_CASE("SupportedLanguages: normalize preserves supported names", "[config][language]")
{
    CHECK(CfgLib::NormalizeSupportedLanguage("German") == "German");
    CHECK(CfgLib::NormalizeSupportedLanguage("polish") == "Polish");
}

TEST_CASE("SupportedLanguages: normalize falls back safely", "[config][language]")
{
    CHECK(CfgLib::NormalizeSupportedLanguage("Esperanto", "Czech") == "Czech");
    CHECK(CfgLib::NormalizeSupportedLanguage("Esperanto", "Esperanto") == "English");
    CHECK(CfgLib::NormalizeSupportedLanguage("Korean", "Russian") == "Russian");
}
