// Unit tests for PoseidonBase platform path utilities

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/platform.hpp>
#include <string.h>
#include <string>

TEST_CASE("PATH_SEP is correct for platform", "[platform]")
{
#ifdef _WIN32
    REQUIRE(PATH_SEP == '\\');
#else
    REQUIRE(PATH_SEP == '/');
#endif
}

TEST_CASE("platformPath (char*) normalizes separators", "[platform]")
{
    SECTION("backslashes to forward slashes on Linux")
    {
        char path[] = "MPMissions\\__cur_mp.eden\\";
        platformPath(path);
#ifdef _WIN32
        REQUIRE(strcmp(path, "MPMissions\\__cur_mp.eden\\") == 0);
#else
        REQUIRE(strcmp(path, "MPMissions/__cur_mp.eden/") == 0);
#endif
    }

    SECTION("forward slashes to backslashes on Windows")
    {
        char path[] = "MPMissions/__cur_mp.eden/";
        platformPath(path);
#ifdef _WIN32
        REQUIRE(strcmp(path, "MPMissions\\__cur_mp.eden\\") == 0);
#else
        REQUIRE(strcmp(path, "MPMissions/__cur_mp.eden/") == 0);
#endif
    }

    SECTION("mixed separators")
    {
        char path[] = "Users\\retro/Saved\\mpmissions/file.pbo";
        platformPath(path);
#ifdef _WIN32
        REQUIRE(strcmp(path, "Users\\retro\\Saved\\mpmissions\\file.pbo") == 0);
#else
        REQUIRE(strcmp(path, "Users/retro/Saved/mpmissions/file.pbo") == 0);
#endif
    }

    SECTION("empty string is safe")
    {
        char path[] = "";
        platformPath(path);
        REQUIRE(strcmp(path, "") == 0);
    }

    SECTION("null pointer is safe")
    {
        platformPath(nullptr);
    }

    SECTION("no separators unchanged")
    {
        char path[] = "file.pbo";
        platformPath(path);
        REQUIRE(strcmp(path, "file.pbo") == 0);
    }
}

TEST_CASE("platformPath (std::string) normalizes separators", "[platform]")
{
    SECTION("backslashes normalized")
    {
        std::string result = platformPath(std::string("MPMissions\\test.eden\\mission.sqm"));
#ifdef _WIN32
        REQUIRE(result == "MPMissions\\test.eden\\mission.sqm");
#else
        REQUIRE(result == "MPMissions/test.eden/mission.sqm");
#endif
    }

    SECTION("forward slashes normalized")
    {
        std::string result = platformPath(std::string("MPMissions/test.eden/mission.sqm"));
#ifdef _WIN32
        REQUIRE(result == "MPMissions\\test.eden\\mission.sqm");
#else
        REQUIRE(result == "MPMissions/test.eden/mission.sqm");
#endif
    }

    SECTION("empty string returns empty")
    {
        REQUIRE(platformPath(std::string("")).empty());
    }
}
