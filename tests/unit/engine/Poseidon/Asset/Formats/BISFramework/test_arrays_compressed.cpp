#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <Poseidon/Asset/Formats/BISStructures.hpp>
#include "test_helpers.hpp"
#include <cstring>
#include <stdint.h>
#include <vector>

using namespace Poseidon::Asset::Formats;
using Catch::Approx;
namespace bis = Poseidon::Asset::Formats; // qualify Vector3 vs the prelude's Foundation::Vector3P

// Arrays with total size < 1024 bytes should be stored uncompressed
TEST_CASE("BISBinaryStream: Compressed arrays below threshold", "[bis][framework][arrays][compressed][small]")
{
    SECTION("Small uint8 array (1023 bytes)")
    {
        std::vector<uint8_t> data;
        uint32_t count = 1023;
        writeValue(data, count);

        for (uint32_t i = 0; i < count; ++i)
            writeValue(data, static_cast<uint8_t>(i % 256));

        TestQIStream stream(data);
        BinaryReader reader(stream);
        std::vector<uint8_t> arr = readCompressedArray<uint8_t>(reader);

        REQUIRE(arr.size() == count);
        REQUIRE(arr[0] == 0);
        REQUIRE(arr[count / 2] == static_cast<uint8_t>((count / 2) % 256));
        REQUIRE(arr[count - 1] == static_cast<uint8_t>((count - 1) % 256));
        // bulk verify all elements match
        std::vector<uint8_t> expected(count);
        for (uint32_t i = 0; i < count; ++i)
            expected[i] = static_cast<uint8_t>(i % 256);
        REQUIRE(memcmp(arr.data(), expected.data(), count) == 0);
    }

    SECTION("Small uint16 array (511 elements = 1022 bytes)")
    {
        std::vector<uint8_t> data;
        uint32_t count = 511;
        writeValue(data, count);

        for (uint32_t i = 0; i < count; ++i)
            writeValue(data, static_cast<uint16_t>(i));

        TestQIStream stream(data);
        BinaryReader reader(stream);
        std::vector<uint16_t> arr = readCompressedArray<uint16_t>(reader);

        REQUIRE(arr.size() == count);
        REQUIRE(arr[0] == 0);
        REQUIRE(arr[count / 2] == static_cast<uint16_t>(count / 2));
        REQUIRE(arr[count - 1] == static_cast<uint16_t>(count - 1));
        std::vector<uint16_t> expected(count);
        for (uint32_t i = 0; i < count; ++i)
            expected[i] = static_cast<uint16_t>(i);
        REQUIRE(memcmp(arr.data(), expected.data(), count * sizeof(uint16_t)) == 0);
    }

    SECTION("Small uint32 array (255 elements = 1020 bytes)")
    {
        std::vector<uint8_t> data;
        uint32_t count = 255;
        writeValue(data, count);

        for (uint32_t i = 0; i < count; ++i)
            writeValue(data, i * 100);

        TestQIStream stream(data);
        BinaryReader reader(stream);
        std::vector<uint32_t> arr = readCompressedArray<uint32_t>(reader);

        REQUIRE(arr.size() == count);
        REQUIRE(arr[0] == 0);
        REQUIRE(arr[count / 2] == (count / 2) * 100);
        REQUIRE(arr[count - 1] == (count - 1) * 100);
        std::vector<uint32_t> expected(count);
        for (uint32_t i = 0; i < count; ++i)
            expected[i] = i * 100;
        REQUIRE(memcmp(arr.data(), expected.data(), count * sizeof(uint32_t)) == 0);
    }
}

// Arrays >= 1024 bytes use LZSS compression - test actual compression/decompression
TEST_CASE("BISBinaryStream: Compressed arrays above threshold", "[bis][framework][arrays][compressed][large]")
{
    SECTION("Large uint8 array (2048 bytes)")
    {
        std::vector<uint8_t> data;
        uint32_t count = 2048;

        std::vector<uint8_t> uncompressed;
        for (uint32_t i = 0; i < count; ++i)
            uncompressed.push_back(static_cast<uint8_t>(i % 256));

        std::vector<uint8_t> compressed = compressData(uncompressed);
        writeValue(data, count);
        data.insert(data.end(), compressed.begin(), compressed.end());

        TestQIStream stream(data);
        BinaryReader reader(stream);
        std::vector<uint8_t> arr = readCompressedArray<uint8_t>(reader);

        REQUIRE(arr.size() == count);
        REQUIRE(arr[0] == 0);
        REQUIRE(arr[255] == 255);
        REQUIRE(arr[count / 2] == static_cast<uint8_t>((count / 2) % 256));
        REQUIRE(arr[count - 1] == static_cast<uint8_t>((count - 1) % 256));
        REQUIRE(memcmp(arr.data(), uncompressed.data(), count) == 0);
    }

    SECTION("Large uint16 array (1024 elements = 2048 bytes)")
    {
        std::vector<uint8_t> data;
        uint32_t count = 1024;

        std::vector<uint8_t> uncompressed;
        for (uint32_t i = 0; i < count; ++i)
        {
            uint16_t value = static_cast<uint16_t>(i);
            uncompressed.push_back(value & 0xFF);
            uncompressed.push_back((value >> 8) & 0xFF);
        }

        std::vector<uint8_t> compressed = compressData(uncompressed);
        writeValue(data, count);
        data.insert(data.end(), compressed.begin(), compressed.end());

        TestQIStream stream(data);
        BinaryReader reader(stream);
        std::vector<uint16_t> arr = readCompressedArray<uint16_t>(reader);

        REQUIRE(arr.size() == count);
        REQUIRE(arr[0] == 0);
        REQUIRE(arr[count / 2] == static_cast<uint16_t>(count / 2));
        REQUIRE(arr[count - 1] == static_cast<uint16_t>(count - 1));
        REQUIRE(memcmp(arr.data(), uncompressed.data(), count * sizeof(uint16_t)) == 0);
    }

    SECTION("Large uint32 array (512 elements = 2048 bytes)")
    {
        std::vector<uint8_t> data;
        uint32_t count = 512;

        std::vector<uint8_t> uncompressed;
        for (uint32_t i = 0; i < count; ++i)
        {
            uint32_t value = i * 1000;
            uncompressed.push_back(value & 0xFF);
            uncompressed.push_back((value >> 8) & 0xFF);
            uncompressed.push_back((value >> 16) & 0xFF);
            uncompressed.push_back((value >> 24) & 0xFF);
        }

        std::vector<uint8_t> compressed = compressData(uncompressed);
        writeValue(data, count);
        data.insert(data.end(), compressed.begin(), compressed.end());

        TestQIStream stream(data);
        BinaryReader reader(stream);
        std::vector<uint32_t> arr = readCompressedArray<uint32_t>(reader);

        REQUIRE(arr.size() == count);
        REQUIRE(arr[0] == 0);
        REQUIRE(arr[count / 2] == (count / 2) * 1000);
        REQUIRE(arr[count - 1] == (count - 1) * 1000);
        REQUIRE(memcmp(arr.data(), uncompressed.data(), count * sizeof(uint32_t)) == 0);
    }

    SECTION("Large float array (512 elements = 2048 bytes)")
    {
        std::vector<uint8_t> data;
        uint32_t count = 512;

        std::vector<uint8_t> uncompressed;
        for (uint32_t i = 0; i < count; ++i)
        {
            float value = static_cast<float>(i) * 0.1f;
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
            uncompressed.insert(uncompressed.end(), bytes, bytes + sizeof(float));
        }

        std::vector<uint8_t> compressed = compressData(uncompressed);
        writeValue(data, count);
        data.insert(data.end(), compressed.begin(), compressed.end());

        TestQIStream stream(data);
        BinaryReader reader(stream);
        std::vector<float> arr = readCompressedArray<float>(reader);

        REQUIRE(arr.size() == count);
        REQUIRE(arr[0] == Approx(0.0f));
        REQUIRE(arr[count / 2] == Approx(static_cast<float>(count / 2) * 0.1f));
        REQUIRE(arr[count - 1] == Approx(static_cast<float>(count - 1) * 0.1f));
        REQUIRE(memcmp(arr.data(), uncompressed.data(), count * sizeof(float)) == 0);
    }

    SECTION("Large Vector3 array (350 elements = 4200 bytes)")
    {
        std::vector<uint8_t> data;
        uint32_t count = 350;

        std::vector<uint8_t> uncompressed;
        for (uint32_t i = 0; i < count; ++i)
        {
            float x = static_cast<float>(i) * 1.0f;
            float y = static_cast<float>(i) * 2.0f;
            float z = static_cast<float>(i) * 3.0f;

            const uint8_t* xBytes = reinterpret_cast<const uint8_t*>(&x);
            const uint8_t* yBytes = reinterpret_cast<const uint8_t*>(&y);
            const uint8_t* zBytes = reinterpret_cast<const uint8_t*>(&z);

            uncompressed.insert(uncompressed.end(), xBytes, xBytes + sizeof(float));
            uncompressed.insert(uncompressed.end(), yBytes, yBytes + sizeof(float));
            uncompressed.insert(uncompressed.end(), zBytes, zBytes + sizeof(float));
        }

        std::vector<uint8_t> compressed = compressData(uncompressed);
        writeValue(data, count);
        data.insert(data.end(), compressed.begin(), compressed.end());

        TestQIStream stream(data);
        BinaryReader reader(stream);
        std::vector<bis::Vector3> arr = readCompressedArray<bis::Vector3>(reader);

        REQUIRE(arr.size() == count);
        REQUIRE(arr[0].x == Approx(0.0f));
        REQUIRE(arr[0].y == Approx(0.0f));
        REQUIRE(arr[0].z == Approx(0.0f));
        REQUIRE(arr[count / 2].x == Approx(static_cast<float>(count / 2) * 1.0f));
        REQUIRE(arr[count / 2].y == Approx(static_cast<float>(count / 2) * 2.0f));
        REQUIRE(arr[count / 2].z == Approx(static_cast<float>(count / 2) * 3.0f));
        REQUIRE(arr[count - 1].x == Approx(static_cast<float>(count - 1) * 1.0f));
        REQUIRE(arr[count - 1].y == Approx(static_cast<float>(count - 1) * 2.0f));
        REQUIRE(arr[count - 1].z == Approx(static_cast<float>(count - 1) * 3.0f));
        REQUIRE(memcmp(arr.data(), uncompressed.data(), count * sizeof(bis::Vector3)) == 0);
    }
}

// Arrays with exactly 1024 bytes should trigger compression
TEST_CASE("BISBinaryStream: Compressed arrays at threshold", "[bis][framework][arrays][compressed][threshold]")
{
    SECTION("uint8 array exactly at threshold (1024 bytes)")
    {
        std::vector<uint8_t> data;
        uint32_t count = 1024;

        std::vector<uint8_t> uncompressed;
        for (uint32_t i = 0; i < count; ++i)
            uncompressed.push_back(static_cast<uint8_t>(i % 256));

        std::vector<uint8_t> compressed = compressData(uncompressed);
        writeValue(data, count);
        data.insert(data.end(), compressed.begin(), compressed.end());

        TestQIStream stream(data);
        BinaryReader reader(stream);
        std::vector<uint8_t> arr = readCompressedArray<uint8_t>(reader);

        REQUIRE(arr.size() == count);
        REQUIRE(arr[0] == 0);
        REQUIRE(arr[count / 2] == static_cast<uint8_t>((count / 2) % 256));
        REQUIRE(arr[count - 1] == static_cast<uint8_t>((count - 1) % 256));
        REQUIRE(memcmp(arr.data(), uncompressed.data(), count) == 0);
    }

    SECTION("uint16 array exactly at threshold (512 elements = 1024 bytes)")
    {
        std::vector<uint8_t> data;
        uint32_t count = 512;

        std::vector<uint8_t> uncompressed;
        for (uint32_t i = 0; i < count; ++i)
        {
            uint16_t value = static_cast<uint16_t>(i);
            uncompressed.push_back(value & 0xFF);
            uncompressed.push_back((value >> 8) & 0xFF);
        }

        std::vector<uint8_t> compressed = compressData(uncompressed);
        writeValue(data, count);
        data.insert(data.end(), compressed.begin(), compressed.end());

        TestQIStream stream(data);
        BinaryReader reader(stream);
        std::vector<uint16_t> arr = readCompressedArray<uint16_t>(reader);

        REQUIRE(arr.size() == count);
        REQUIRE(arr[0] == 0);
        REQUIRE(arr[count / 2] == static_cast<uint16_t>(count / 2));
        REQUIRE(arr[count - 1] == static_cast<uint16_t>(count - 1));
        REQUIRE(memcmp(arr.data(), uncompressed.data(), count * sizeof(uint16_t)) == 0);
    }

    SECTION("uint32 array exactly at threshold (256 elements = 1024 bytes)")
    {
        std::vector<uint8_t> data;
        uint32_t count = 256;

        std::vector<uint8_t> uncompressed;
        for (uint32_t i = 0; i < count; ++i)
        {
            uint32_t value = i * 100;
            uncompressed.push_back(value & 0xFF);
            uncompressed.push_back((value >> 8) & 0xFF);
            uncompressed.push_back((value >> 16) & 0xFF);
            uncompressed.push_back((value >> 24) & 0xFF);
        }

        std::vector<uint8_t> compressed = compressData(uncompressed);
        writeValue(data, count);
        data.insert(data.end(), compressed.begin(), compressed.end());

        TestQIStream stream(data);
        BinaryReader reader(stream);
        std::vector<uint32_t> arr = readCompressedArray<uint32_t>(reader);

        REQUIRE(arr.size() == count);
        REQUIRE(arr[0] == 0);
        REQUIRE(arr[count / 2] == (count / 2) * 100);
        REQUIRE(arr[count - 1] == (count - 1) * 100);
        REQUIRE(memcmp(arr.data(), uncompressed.data(), count * sizeof(uint32_t)) == 0);
    }
}
