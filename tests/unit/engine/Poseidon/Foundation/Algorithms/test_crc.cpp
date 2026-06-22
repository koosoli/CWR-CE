#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Algorithms/Crc.hpp>
#include <stdint.h>
#include <string.h>

TEST_CASE("CRCCalculator - known vectors", "[utility][crc]")
{
    Poseidon::Foundation::CRCCalculator crc;

    SECTION("known one-shot vectors")
    {
        uint32_t result = crc.CRC("", 0);
        REQUIRE(result == 0x00000000u);
        REQUIRE(crc.CRC("A", 1) == 0x81B02D8Bu);
        REQUIRE(crc.CRC("123456789", 9) == 0xFC891918u);
    }

    SECTION("single byte CRC")
    {
        uint32_t result = crc.CRC("A", 1);
        REQUIRE(result != 0);
        // Same input gives same output
        uint32_t result2 = crc.CRC("A", 1);
        REQUIRE(result == result2);
    }

    SECTION("different inputs give different CRCs")
    {
        uint32_t crc1 = crc.CRC("hello", 5);
        uint32_t crc2 = crc.CRC("world", 5);
        REQUIRE(crc1 != crc2);
    }

    SECTION("incremental interface matches one-shot")
    {
        const char* data = "test data for CRC";
        int len = (int)strlen(data);

        uint32_t oneshot = crc.CRC(data, len);

        crc.Reset();
        crc.Add(data, len);
        uint32_t incremental = crc.GetResult();

        REQUIRE(oneshot == incremental);
    }

    SECTION("incremental byte-by-byte matches one-shot")
    {
        const char* data = "abcdef";
        int len = 6;

        uint32_t oneshot = crc.CRC(data, len);

        crc.Reset();
        for (int i = 0; i < len; i++)
        {
            crc.Add(data[i]);
        }
        uint32_t incremental = crc.GetResult();

        REQUIRE(oneshot == incremental);
    }
}
