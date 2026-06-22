#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <Poseidon/Asset/Formats/BISStructures.hpp>
#include "test_helpers.hpp"
#include <stdint.h>
#include <vector>

using namespace Poseidon::Asset::Formats;
using Catch::Approx;

TEST_CASE("BISBinaryStream: P3D integration", "[bis][framework][p3d][integration]")
{
    SECTION("Read P3D header")
    {
        std::vector<uint8_t> data;
        writeValue(data, static_cast<uint32_t>(0x4C4F444F)); // 'ODOL'
        writeValue(data, static_cast<uint32_t>(7));          // version 7
        writeValue(data, static_cast<uint32_t>(2));          // 2 LODs

        TestQIStream stream(data);
        BinaryReader reader(stream);

        uint32_t signature = reader.read<uint32_t>();
        uint32_t version = reader.read<uint32_t>();
        uint32_t lodCount = reader.read<uint32_t>();

        REQUIRE(signature == 0x4C4F444F);
        REQUIRE(version == 7);
        REQUIRE(lodCount == 2);
    }

    SECTION("Read LOD vertex table components")
    {
        std::vector<uint8_t> data;

        // Point flags (3 elements, uncompressed)
        writeValue(data, static_cast<uint32_t>(3));
        writeValue(data, static_cast<uint32_t>(0x01));
        writeValue(data, static_cast<uint32_t>(0x02));
        writeValue(data, static_cast<uint32_t>(0x04));

        // UV coords (2 elements, uncompressed)
        writeValue(data, static_cast<uint32_t>(2));
        writeValue(data, 0.0f);
        writeValue(data, 0.0f);
        writeValue(data, 1.0f);
        writeValue(data, 1.0f);

        TestQIStream stream(data);
        BinaryReader reader(stream);

        // Read point flags
        std::vector<uint32_t> pointFlags = readCompressedArray<uint32_t>(reader);
        REQUIRE(pointFlags.size() == 3);
        REQUIRE(pointFlags[0] == 0x01);
        REQUIRE(pointFlags[1] == 0x02);
        REQUIRE(pointFlags[2] == 0x04);

        // Read UV coords
        std::vector<Vector2> uvCoords = readCompressedArray<Vector2>(reader);
        REQUIRE(uvCoords.size() == 2);
        REQUIRE(uvCoords[0].u == Approx(0.0f));
        REQUIRE(uvCoords[0].v == Approx(0.0f));
        REQUIRE(uvCoords[1].u == Approx(1.0f));
        REQUIRE(uvCoords[1].v == Approx(1.0f));
    }
}

TEST_CASE("BISBinaryStream: Configurable compression threshold", "[bis][framework][threshold][config]")
{
    SECTION("Verify default threshold")
    {
        REQUIRE(COMPRESSION_THRESHOLD == 1024);
    }

    SECTION("Calculate threshold for different types")
    {
        // uint8: 1024 elements
        REQUIRE(1024 * sizeof(uint8_t) >= COMPRESSION_THRESHOLD);
        REQUIRE(1023 * sizeof(uint8_t) < COMPRESSION_THRESHOLD);

        // uint16: 512 elements
        REQUIRE(512 * sizeof(uint16_t) >= COMPRESSION_THRESHOLD);
        REQUIRE(511 * sizeof(uint16_t) < COMPRESSION_THRESHOLD);

        // uint32: 256 elements
        REQUIRE(256 * sizeof(uint32_t) >= COMPRESSION_THRESHOLD);
        REQUIRE(255 * sizeof(uint32_t) < COMPRESSION_THRESHOLD);

        // float: 256 elements
        REQUIRE(256 * sizeof(float) >= COMPRESSION_THRESHOLD);
        REQUIRE(255 * sizeof(float) < COMPRESSION_THRESHOLD);

        // Vector2: 128 elements
        REQUIRE(128 * sizeof(Vector2) >= COMPRESSION_THRESHOLD);
        REQUIRE(127 * sizeof(Vector2) < COMPRESSION_THRESHOLD);
    }
}
