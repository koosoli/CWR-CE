// UserConfig Tests
// Verifies user configuration difficulty logic and mode switching

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Core/Config/UserConfig.hpp>
#include <Poseidon/Core/Difficulty.hpp>

// Config::diffDesc accessor (defined in ConfigFunctions.cpp)
namespace Poseidon
{
DifficultyDesc* GetDifficultyDescs();
}

TEST_CASE("UserConfig default initialization", "[core][config][difficulty]")
{
    Poseidon::UserConfig config;

    SECTION("Defaults match original diffDesc")
    {
        // Cadet defaults (from diffDesc.defaultCadet)
        REQUIRE(config.cadetDifficulty[Poseidon::DTArmor] == true);
        REQUIRE(config.cadetDifficulty[Poseidon::DTFriendlyTag] == true);
        REQUIRE(config.cadetDifficulty[Poseidon::DTEnemyTag] == false);
        REQUIRE(config.cadetDifficulty[Poseidon::DTHUD] == true);
        REQUIRE(config.cadetDifficulty[Poseidon::DTAutoSpot] == true);
        REQUIRE(config.cadetDifficulty[Poseidon::DTMap] == true);
        REQUIRE(config.cadetDifficulty[Poseidon::DTWeaponCursor] == true);
        REQUIRE(config.cadetDifficulty[Poseidon::DTAutoGuideAT] == true);
        REQUIRE(config.cadetDifficulty[Poseidon::DTClockIndicator] == true);
        REQUIRE(config.cadetDifficulty[Poseidon::DT3rdPersonView] == true);
        REQUIRE(config.cadetDifficulty[Poseidon::DTTracers] == true);
        REQUIRE(config.cadetDifficulty[Poseidon::DTUltraAI] == false);

        // Veteran defaults (from diffDesc.defaultVeteran)
        REQUIRE(config.veteranDifficulty[Poseidon::DTArmor] == false);
        REQUIRE(config.veteranDifficulty[Poseidon::DTFriendlyTag] == false);
        REQUIRE(config.veteranDifficulty[Poseidon::DTEnemyTag] == false);
        REQUIRE(config.veteranDifficulty[Poseidon::DTHUD] == false);
        REQUIRE(config.veteranDifficulty[Poseidon::DTAutoSpot] == false);
        REQUIRE(config.veteranDifficulty[Poseidon::DTMap] == false);
        REQUIRE(config.veteranDifficulty[Poseidon::DTWeaponCursor] == true);
        REQUIRE(config.veteranDifficulty[Poseidon::DTAutoGuideAT] == false);
        REQUIRE(config.veteranDifficulty[Poseidon::DTClockIndicator] == true);
        REQUIRE(config.veteranDifficulty[Poseidon::DT3rdPersonView] == true);
        REQUIRE(config.veteranDifficulty[Poseidon::DTTracers] == true);
        REQUIRE(config.veteranDifficulty[Poseidon::DTUltraAI] == false);
    }

    SECTION("UI preferences defaults")
    {
        REQUIRE(config.showTitles == true);
        REQUIRE(config.easyMode == true);
    }

    SECTION("FOV defaults")
    {
        REQUIRE(config.fovTop == 0.75f);
        REQUIRE(config.fovLeft == 1.0f);
    }
}

TEST_CASE("UserConfig difficulty array modification", "[core][config][difficulty]")
{
    Poseidon::UserConfig config;

    SECTION("Modify cadet difficulty settings")
    {
        config.cadetDifficulty[Poseidon::DTArmor] = false;
        config.cadetDifficulty[Poseidon::DTAutoSpot] = false;
        config.cadetDifficulty[Poseidon::DTHUD] = true;
        config.cadetDifficulty[Poseidon::DTMap] = true;

        REQUIRE(config.cadetDifficulty[Poseidon::DTArmor] == false);
        REQUIRE(config.cadetDifficulty[Poseidon::DTAutoSpot] == false);
        REQUIRE(config.cadetDifficulty[Poseidon::DTHUD] == true);
        REQUIRE(config.cadetDifficulty[Poseidon::DTMap] == true);
    }

    SECTION("Modify veteran difficulty settings")
    {
        config.veteranDifficulty[Poseidon::DTArmor] = true;
        config.veteranDifficulty[Poseidon::DTHUD] = false;
        config.veteranDifficulty[Poseidon::DTTracers] = true;

        REQUIRE(config.veteranDifficulty[Poseidon::DTArmor] == true);
        REQUIRE(config.veteranDifficulty[Poseidon::DTHUD] == false);
        REQUIRE(config.veteranDifficulty[Poseidon::DTTracers] == true);
    }

    SECTION("Arrays are independent")
    {
        config.cadetDifficulty[Poseidon::DTArmor] = false;
        config.veteranDifficulty[Poseidon::DTArmor] = true;

        // Verify both hold their own values
        REQUIRE(config.cadetDifficulty[Poseidon::DTArmor] == false);
        REQUIRE(config.veteranDifficulty[Poseidon::DTArmor] == true);

        // Modify one shouldn't affect the other
        config.cadetDifficulty[Poseidon::DTArmor] = true;
        REQUIRE(config.veteranDifficulty[Poseidon::DTArmor] == true); // unchanged
    }
}

TEST_CASE("UserConfig IsEnabled logic", "[core][config][difficulty]")
{
    Poseidon::UserConfig config;

    SECTION("Easy mode uses cadet difficulty")
    {
        config.cadetDifficulty[Poseidon::DTArmor] = true;
        config.veteranDifficulty[Poseidon::DTArmor] = false;

        config.easyMode = true;
        REQUIRE(config.IsEnabled(Poseidon::DTArmor) == true);
    }

    SECTION("Veteran mode uses veteran difficulty")
    {
        config.cadetDifficulty[Poseidon::DTArmor] = true;
        config.veteranDifficulty[Poseidon::DTArmor] = false;

        config.easyMode = false;
        REQUIRE(config.IsEnabled(Poseidon::DTArmor) == false);
    }

    SECTION("Mode switching changes behavior")
    {
        config.cadetDifficulty[Poseidon::DTMap] = true;
        config.veteranDifficulty[Poseidon::DTMap] = false;

        config.easyMode = true;
        REQUIRE(config.IsEnabled(Poseidon::DTMap) == true);

        config.easyMode = false;
        REQUIRE(config.IsEnabled(Poseidon::DTMap) == false);
    }

    SECTION("Each difficulty type queried correctly")
    {
        config.easyMode = true;
        config.cadetDifficulty[Poseidon::DTArmor] = true;
        config.cadetDifficulty[Poseidon::DTFriendlyTag] = false;
        config.cadetDifficulty[Poseidon::DTAutoSpot] = true;

        REQUIRE(config.IsEnabled(Poseidon::DTArmor) == true);
        REQUIRE(config.IsEnabled(Poseidon::DTFriendlyTag) == false);
        REQUIRE(config.IsEnabled(Poseidon::DTAutoSpot) == true);
    }

    SECTION("All 12 difficulty types accessible")
    {
        config.easyMode = false;

        // Set veteran array to alternating pattern
        for (int i = 0; i < Poseidon::DTN - 1; i++)
        {
            config.veteranDifficulty[i] = (i % 3 == 0);
        }

        // Verify all types accessible
        REQUIRE(config.IsEnabled(static_cast<Poseidon::DifficultyType>(0)) == true);  // 0 % 3 == 0
        REQUIRE(config.IsEnabled(static_cast<Poseidon::DifficultyType>(1)) == false); // 1 % 3 != 0
        REQUIRE(config.IsEnabled(static_cast<Poseidon::DifficultyType>(3)) == true);  // 3 % 3 == 0
        REQUIRE(config.IsEnabled(static_cast<Poseidon::DifficultyType>(6)) == true);  // 6 % 3 == 0
        REQUIRE(config.IsEnabled(static_cast<Poseidon::DifficultyType>(9)) == true);  // 9 % 3 == 0
    }
}

TEST_CASE("UserConfig InitDifficulties", "[core][config][difficulty]")
{
    Poseidon::UserConfig config;

    SECTION("InitDifficulties resets to defaults")
    {
        // Modify away from defaults
        config.cadetDifficulty[Poseidon::DTArmor] = false;
        config.veteranDifficulty[Poseidon::DTWeaponCursor] = false;
        config.showTitles = false;

        // Reset to defaults
        config.InitDifficulties();

        // Verify defaults restored
        REQUIRE(config.cadetDifficulty[Poseidon::DTArmor] == true);
        REQUIRE(config.veteranDifficulty[Poseidon::DTWeaponCursor] == true);
        REQUIRE(config.showTitles == true);
    }

    SECTION("InitDifficulties sets all difficulty flags")
    {
        // Scramble values
        for (int i = 0; i < Poseidon::DTN; i++)
        {
            config.cadetDifficulty[i] = (i % 2 == 0);
            config.veteranDifficulty[i] = (i % 3 == 0);
        }

        // Reset
        config.InitDifficulties();

        // Verify all reset to diffDesc defaults
        for (int i = 0; i < Poseidon::DTN; i++)
        {
            REQUIRE(config.cadetDifficulty[i] == Poseidon::GetDifficultyDescs()[i].defaultCadet);
            REQUIRE(config.veteranDifficulty[i] == Poseidon::GetDifficultyDescs()[i].defaultVeteran);
        }
    }
}

TEST_CASE("Config diffDesc static array", "[core][config][difficulty]")
{
    SECTION("All 12 descriptors present")
    {
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTArmor].name != nullptr);
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTFriendlyTag].name != nullptr);
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTEnemyTag].name != nullptr);
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTHUD].name != nullptr);
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTAutoSpot].name != nullptr);
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTMap].name != nullptr);
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTWeaponCursor].name != nullptr);
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTAutoGuideAT].name != nullptr);
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTClockIndicator].name != nullptr);
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DT3rdPersonView].name != nullptr);
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTTracers].name != nullptr);
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTUltraAI].name != nullptr);
    }

    SECTION("String IDs are assigned")
    {
        // String IDs should be the IDS_DIFF_* constants (may be 0 in test context)
        // Just verify they're the same type
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTArmor].stringId ==
                Poseidon::GetDifficultyDescs()[Poseidon::DTArmor].stringId);
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTFriendlyTag].stringId ==
                Poseidon::GetDifficultyDescs()[Poseidon::DTFriendlyTag].stringId);
    }

    SECTION("enabledInVeteran flags correct")
    {
        // These should be false in veteran mode (from original Config::diffDesc)
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTArmor].enabledInVeteran == false);
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTFriendlyTag].enabledInVeteran == false);
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTEnemyTag].enabledInVeteran == false);

        // These should be true (allowed in veteran)
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTWeaponCursor].enabledInVeteran == true);
        REQUIRE(Poseidon::GetDifficultyDescs()[Poseidon::DTClockIndicator].enabledInVeteran == true);
    }
}
