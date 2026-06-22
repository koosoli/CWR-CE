// Unit tests for CRC32 algorithm from PoseidonBase

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Algorithms/Crc32.h>
#include <string.h>
#include <Poseidon/Foundation/Common/Global.hpp>

using Poseidon::Foundation::crc32;

TEST_CASE("CRC32 basic functionality", "[crc32]")
{
    SECTION("CRC32 of empty data")
    {
        // CRC32 of null pointer should return 0
        unsigned32 result = crc32(0, nullptr, 0);
        REQUIRE(result == 0);
    }

    SECTION("CRC32 of simple string")
    {
        const char* testData = "Hello, World!";
        unsigned32 crc = crc32(0, (const unsigned8*)testData, strlen(testData));

        // CRC32 should be deterministic
        unsigned32 crc2 = crc32(0, (const unsigned8*)testData, strlen(testData));
        REQUIRE(crc == crc2);

        // CRC32 should be non-zero for non-empty data
        REQUIRE(crc != 0);
    }

    SECTION("CRC32 with initial value")
    {
        const char* data1 = "Hello";
        const char* data2 = ", World!";

        // Calculate CRC32 in one go
        unsigned32 crcCombined = crc32(0, (const unsigned8*)"Hello, World!", 13);

        // Calculate CRC32 incrementally
        unsigned32 crc1 = crc32(0, (const unsigned8*)data1, 5);
        unsigned32 crc2 = crc32(crc1, (const unsigned8*)data2, 8);

        // Both should give the same result
        REQUIRE(crc2 == crcCombined);
    }

    SECTION("CRC32 is sensitive to data changes")
    {
        const char* data1 = "test";
        const char* data2 = "Test"; // Capital T

        unsigned32 crc1 = crc32(0, (const unsigned8*)data1, 4);
        unsigned32 crc2 = crc32(0, (const unsigned8*)data2, 4);

        // Different input should give different CRC
        REQUIRE(crc1 != crc2);
    }

    SECTION("CRC32 known test vector")
    {
        // Test with known CRC32 value
        // CRC32 of "123456789" is 0xCBF43926
        const char* testData = "123456789";
        unsigned32 crc = crc32(0, (const unsigned8*)testData, 9);

        REQUIRE(crc == 0xCBF43926);
    }
}
