#include <catch2/catch_test_macros.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include <Poseidon/World/Terrain/WrpReader.hpp>
#include "test_fixtures.hpp"
#include <stdio.h>
#include <catch2/catch_message.hpp>
#include <string>
#include <Poseidon/Foundation/platform.hpp>

using namespace Poseidon;

#ifdef GetObject
#undef GetObject
#endif

// OPRW magic used by serialized landscape binaries
#ifdef _MSC_VER
static const int OPRW_MAGIC = 'WRPO';
#else
static const int OPRW_MAGIC = StrToInt("OPRW");
#endif

TEST_CASE("WRP: synthetic placeholder is rejected as non-OPRW", "[Formats][WRP]")
{
    const char* path = GET_FIXTURE("wrp/test_world.wrp");

    QIFStream file;
    file.open(path);
    REQUIRE(!file.fail());

    int magic = 0;
    file.read(&magic, sizeof(magic));
    REQUIRE(!file.fail());
    REQUIRE(magic == OPRW_MAGIC);
}

TEST_CASE("WRP: WrpReader rejects synthetic placeholder terrain", "[Formats][WRP]")
{
    const char* path = GET_FIXTURE("wrp/test_world.wrp");

    WrpReader reader;
    REQUIRE_FALSE(reader.Load(path));
    REQUIRE(reader.GetError() != nullptr);
}

TEST_CASE("WRP: WrpReader loads large OPRW with objects", "[Formats][WRP][GameData]")
{
    // Game data lives under GAME_DATA_DIR/Worlds/.
    char abelPath[MAX_PATH];
    snprintf(abelPath, sizeof(abelPath), "%s/Worlds/abel.wrp", GAME_DATA_DIR);

    QIFStream probe;
    probe.open(abelPath);
    if (probe.fail())
    {
        SKIP("Game world abel.wrp not available");
        return;
    }

    WrpReader reader;
    REQUIRE(reader.Load(abelPath));
    REQUIRE((reader.GetFormat() == WrpReader::OPRW_V2 || reader.GetFormat() == WrpReader::OPRW_V3));
    REQUIRE(reader.GetObjectCount() > 0);
    REQUIRE(reader.GetObjectNameCount() > 0);
    REQUIRE(reader.GetTextureCount() > 0);

    // Verify first object has valid data
    const auto& obj = reader.GetObject(0);
    REQUIRE(obj.name.GetLength() > 0);
    REQUIRE(obj.hasMatrix == true);
}

TEST_CASE("WRP: WrpReader reports error for missing file", "[Formats][WRP]")
{
    WrpReader reader;
    REQUIRE_FALSE(reader.Load("nonexistent.wrp"));
    REQUIRE(reader.GetError() != nullptr);
}

TEST_CASE("WRP: WrpReader reports error for invalid data", "[Formats][WRP]")
{
    // Create a local temp file with invalid magic. Keep this relative so it
    // works under Windows ctest runs without relying on /tmp.
    const char* tmpPath = "test_invalid.wrp";
    {
        QOFStream f;
        f.open(tmpPath);
        int badMagic = 0xDEADBEEF;
        f.write(&badMagic, sizeof(badMagic));
        f.close();
    }

    WrpReader reader;
    REQUIRE_FALSE(reader.Load(tmpPath));
    REQUIRE(std::string(reader.GetError()) == "Unknown file format");

    remove(tmpPath);
}
