#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <Poseidon/Asset/Formats/BISStructures.hpp>
#include "test_helpers.hpp"
#include <stddef.h>
#include <stdint.h>
#include <vector>

using namespace Poseidon::Asset::Formats;
using Catch::Approx;
namespace bis = Poseidon::Asset::Formats; // qualify Vector3 vs the prelude's Foundation::Vector3P

TEST_CASE("BISBinaryStream: Read uncompressed arrays", "[bis][framework][arrays][uncompressed]")
{
    SECTION("Read uint8 array")
    {
        std::vector<uint8_t> data;
        writeValue(data, static_cast<uint32_t>(5));
        for (uint8_t i = 0; i < 5; ++i)
        {
            writeValue(data, static_cast<uint8_t>(10 + i));
        }

        TestQIStream stream(data);
        BinaryReader reader(stream);

        std::vector<uint8_t> arr = readArray<uint8_t>(reader);

        REQUIRE(arr.size() == 5);
        for (size_t i = 0; i < 5; ++i)
        {
            REQUIRE(arr[i] == 10 + i);
        }
    }

    SECTION("Read uint16 array")
    {
        std::vector<uint8_t> data;
        writeValue(data, static_cast<uint32_t>(3));
        writeValue(data, static_cast<uint16_t>(100));
        writeValue(data, static_cast<uint16_t>(200));
        writeValue(data, static_cast<uint16_t>(300));

        TestQIStream stream(data);
        BinaryReader reader(stream);

        std::vector<uint16_t> arr = readArray<uint16_t>(reader);

        REQUIRE(arr.size() == 3);
        REQUIRE(arr[0] == 100);
        REQUIRE(arr[1] == 200);
        REQUIRE(arr[2] == 300);
    }

    SECTION("Read uint32 array")
    {
        std::vector<uint8_t> data;
        writeValue(data, static_cast<uint32_t>(2));
        writeValue(data, static_cast<uint32_t>(1000));
        writeValue(data, static_cast<uint32_t>(2000));

        TestQIStream stream(data);
        BinaryReader reader(stream);

        std::vector<uint32_t> arr = readArray<uint32_t>(reader);

        REQUIRE(arr.size() == 2);
        REQUIRE(arr[0] == 1000);
        REQUIRE(arr[1] == 2000);
    }

    SECTION("Read float array")
    {
        std::vector<uint8_t> data;
        writeValue(data, static_cast<uint32_t>(3));
        writeValue(data, 1.5f);
        writeValue(data, 2.5f);
        writeValue(data, 3.5f);

        TestQIStream stream(data);
        BinaryReader reader(stream);

        std::vector<float> arr = readArray<float>(reader);

        REQUIRE(arr.size() == 3);
        REQUIRE(arr[0] == Approx(1.5f));
        REQUIRE(arr[1] == Approx(2.5f));
        REQUIRE(arr[2] == Approx(3.5f));
    }

    SECTION("Read empty array")
    {
        std::vector<uint8_t> data;
        writeValue(data, static_cast<uint32_t>(0));

        TestQIStream stream(data);
        BinaryReader reader(stream);

        std::vector<uint32_t> arr = readArray<uint32_t>(reader);

        REQUIRE(arr.empty());
    }
}

TEST_CASE("BISBinaryStream: Read Vector3 arrays", "[bis][framework][arrays][vector3]")
{
    SECTION("Read Vector3 array")
    {
        std::vector<uint8_t> data;
        writeValue(data, static_cast<uint32_t>(3));

        // Vector 1
        writeValue(data, 1.0f);
        writeValue(data, 2.0f);
        writeValue(data, 3.0f);

        // Vector 2
        writeValue(data, 4.0f);
        writeValue(data, 5.0f);
        writeValue(data, 6.0f);

        // Vector 3
        writeValue(data, 7.0f);
        writeValue(data, 8.0f);
        writeValue(data, 9.0f);

        TestQIStream stream(data);
        BinaryReader reader(stream);

        std::vector<bis::Vector3> arr = readArray<bis::Vector3>(reader);

        REQUIRE(arr.size() == 3);

        REQUIRE(arr[0].x == Approx(1.0f));
        REQUIRE(arr[0].y == Approx(2.0f));
        REQUIRE(arr[0].z == Approx(3.0f));

        REQUIRE(arr[1].x == Approx(4.0f));
        REQUIRE(arr[1].y == Approx(5.0f));
        REQUIRE(arr[1].z == Approx(6.0f));

        REQUIRE(arr[2].x == Approx(7.0f));
        REQUIRE(arr[2].y == Approx(8.0f));
        REQUIRE(arr[2].z == Approx(9.0f));
    }

    SECTION("Read empty Vector3 array")
    {
        std::vector<uint8_t> data;
        writeValue(data, static_cast<uint32_t>(0));

        TestQIStream stream(data);
        BinaryReader reader(stream);

        std::vector<bis::Vector3> arr = readArray<bis::Vector3>(reader);

        REQUIRE(arr.empty());
    }
}
