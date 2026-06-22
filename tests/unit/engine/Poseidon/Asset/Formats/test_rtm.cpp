#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <Poseidon/Asset/Formats/RTM/RTMReader.hpp>
#include <fstream>
#include <cstring>
#include "test_fixtures.hpp"
#include <stdint.h>
#include <string>
#include <vector>

TEST_CASE("RTM: STOZAR.rtm raw binary header and phase data", "[Formats][RTM]")
{
    const char* path = GET_FIXTURE("rtm/marker_motion.rtm");

    std::ifstream file(path, std::ios::binary);
    REQUIRE(file.good());

    char magic[9] = {};
    file.read(magic, 8);
    REQUIRE(std::string(magic) == "RTM_0101");

    float stepX = 0, stepY = 0, stepZ = 0;
    file.read(reinterpret_cast<char*>(&stepX), sizeof(float));
    file.read(reinterpret_cast<char*>(&stepY), sizeof(float));
    file.read(reinterpret_cast<char*>(&stepZ), sizeof(float));
    REQUIRE(stepX == 0.0f);
    REQUIRE(stepY == 0.0f);
    REQUIRE(stepZ == 0.0f);

    int nAnim = 0, nSel = 0;
    file.read(reinterpret_cast<char*>(&nAnim), sizeof(int));
    file.read(reinterpret_cast<char*>(&nSel), sizeof(int));
    REQUIRE(nAnim == 2);
    REQUIRE(nSel == 2);

    char selName[32] = {};
    file.read(selName, 32);
    REQUIRE(std::string(selName) == "pole");
    file.read(selName, 32);
    REQUIRE(std::string(selName) == "flag");

    float phaseTime = -1.0f;
    file.read(reinterpret_cast<char*>(&phaseTime), sizeof(float));
    REQUIRE(phaseTime == 0.0f);

    file.seekg(2 * (32 + 48), std::ios::cur);

    file.read(reinterpret_cast<char*>(&phaseTime), sizeof(float));
    REQUIRE(phaseTime == 1.0f);
    REQUIRE(file.good());
}

TEST_CASE("RTMReader: read pole.rtm from file", "[Formats][RTM]")
{
    const char* path = GET_FIXTURE("rtm/marker_motion.rtm");
    Poseidon::Asset::Formats::RTMAnimation anim;
    REQUIRE(Poseidon::Asset::Formats::readRTMFromFile(path, anim));

    SECTION("version and step")
    {
        REQUIRE(anim.version == Poseidon::Asset::Formats::RTMVersion::V101);
        REQUIRE(anim.stepX == 0.0f);
        REQUIRE(anim.stepY == 0.0f);
        REQUIRE(anim.stepZ == 0.0f);
    }

    SECTION("bone count and names")
    {
        REQUIRE(anim.boneCount() == 2);
        REQUIRE(anim.boneNames[0] == "pole");
        REQUIRE(anim.boneNames[1] == "flag");
    }

    SECTION("phase count and times")
    {
        REQUIRE(anim.phaseCount() == 2);
        REQUIRE(anim.phases[0].time == 0.0f);
        REQUIRE(anim.phases[1].time == 1.0f);
    }

    SECTION("each phase has transforms for all bones")
    {
        REQUIRE(anim.phases[0].transforms.size() == 2);
        REQUIRE(anim.phases[1].transforms.size() == 2);
    }

    SECTION("phase 0 bone 0 (pole) is near-identity")
    {
        const auto& t = anim.phases[0].transforms[0];
        REQUIRE(t.m[0] == Catch::Approx(1.0f).margin(1e-5f));
        REQUIRE(t.m[4] == Catch::Approx(1.0f).margin(1e-5f));
        REQUIRE(t.m[8] == Catch::Approx(1.0f).margin(1e-5f));
        REQUIRE(t.tx() == Catch::Approx(0.0f).margin(1e-5f));
        REQUIRE(t.ty() == Catch::Approx(0.0f).margin(1e-5f));
        REQUIRE(t.tz() == Catch::Approx(0.0f).margin(1e-5f));
    }
}

TEST_CASE("RTMReader: read pole.rtm from memory", "[Formats][RTM]")
{
    const char* path = GET_FIXTURE("rtm/marker_motion.rtm");
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    REQUIRE(file.good());
    auto size = file.tellg();
    file.seekg(0);
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);

    Poseidon::Asset::Formats::RTMAnimation anim;
    REQUIRE(Poseidon::Asset::Formats::readRTMFromMemory(data.data(), data.size(), anim));
    REQUIRE(anim.boneCount() == 2);
    REQUIRE(anim.phaseCount() == 2);
    REQUIRE(anim.boneNames[0] == "pole");
    REQUIRE(anim.boneNames[1] == "flag");
}

TEST_CASE("RTMReader: readHeader only", "[Formats][RTM]")
{
    const char* path = GET_FIXTURE("rtm/marker_motion.rtm");
    std::ifstream file(path, std::ios::binary);
    REQUIRE(file.good());

    Poseidon::Asset::Formats::RTMAnimation anim;
    REQUIRE(Poseidon::Asset::Formats::readRTMHeader(file, anim));
    REQUIRE(anim.boneCount() == 2);
    REQUIRE(anim.phaseCount() == 2); // count known but data empty
    REQUIRE(anim.phases[0].transforms.empty());
    REQUIRE(anim.phases[1].transforms.empty());

    // Now read phases
    REQUIRE(Poseidon::Asset::Formats::readRTMPhases(file, anim));
    REQUIRE(anim.phases[0].transforms.size() == 2);
    REQUIRE(anim.phases[1].transforms.size() == 2);
    REQUIRE(anim.phases[0].time == 0.0f);
    REQUIRE(anim.phases[1].time == 1.0f);
}

TEST_CASE("RTMReader: invalid data returns false", "[Formats][RTM]")
{
    Poseidon::Asset::Formats::RTMAnimation anim;
    uint8_t garbage[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    REQUIRE_FALSE(Poseidon::Asset::Formats::readRTMFromMemory(garbage, sizeof(garbage), anim));
    REQUIRE(anim.version == Poseidon::Asset::Formats::RTMVersion::Unknown);
}
