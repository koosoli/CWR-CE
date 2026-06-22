#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Platform/GamePaths.hpp>
#include <Poseidon/Foundation/platform.hpp>
#include <string>
#include <Poseidon/Foundation/Common/GamePaths.hpp>

TEST_CASE("GameDirs: directory name constants", "[platform][gameDirs]")
{
    REQUIRE(std::string(GameDirs::MPMissions) == "MPMissions");
    REQUIRE(std::string(GameDirs::MPMissionsCache) == "MPMissionsCache");
    REQUIRE(std::string(GameDirs::Missions) == "Missions");
    REQUIRE(std::string(GameDirs::Users) == "Users");
}

TEST_CASE("GameDirs: MPMissionsPath uses platform separator", "[platform][gameDirs]")
{
    std::string path = GameDirs::MPMissionsPath();
#ifdef _WIN32
    REQUIRE(path == "MPMissions\\");
#else
    REQUIRE(path == "MPMissions/");
#endif
    REQUIRE(path.back() == PATH_SEP);
}

TEST_CASE("GameDirs: MPMissionsCachePath uses platform separator", "[platform][gameDirs]")
{
    std::string path = GameDirs::MPMissionsCachePath();
#ifdef _WIN32
    REQUIRE(path == "MPMissionsCache\\");
#else
    REQUIRE(path == "MPMissionsCache/");
#endif
    REQUIRE(path.back() == PATH_SEP);
}

TEST_CASE("GameDirs: MPCurrentPrefix uses platform separator", "[platform][gameDirs]")
{
    std::string prefix = GameDirs::MPCurrentPrefix();
    std::string expected = std::string("MPMissions") + PATH_SEP + "__cur_mp.";
    REQUIRE(prefix == expected);
    REQUIRE(prefix.find("MPMissions") == 0);
    REQUIRE(prefix.find("__cur_mp.") != std::string::npos);
}
