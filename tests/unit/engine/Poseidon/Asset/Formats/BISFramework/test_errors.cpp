#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include "test_helpers.hpp"
#include <stdint.h>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Poseidon::Asset::Formats;

TEST_CASE("BISBinaryStream: Error handling", "[bis][framework][errors]")
{
    SECTION("Read beyond stream end")
    {
        std::vector<uint8_t> data;
        writeValue(data, static_cast<uint32_t>(42));

        TestQIStream stream(data);
        BinaryReader reader(stream);

        reader.read<uint32_t>(); // Read all data

        // Next read should throw
        REQUIRE_THROWS_AS(reader.read<uint32_t>(), std::runtime_error);
    }

    SECTION("Invalid string length")
    {
        std::vector<uint8_t> data;
        writeValue(data, static_cast<int32_t>(-1)); // Negative length

        TestQIStream stream(data);
        BinaryReader reader(stream);

        REQUIRE_THROWS_AS(readString(reader), std::runtime_error);
    }

    SECTION("String length too large")
    {
        std::vector<uint8_t> data;
        writeValue(data, static_cast<int32_t>(1024 * 1024 + 1)); // > 1MB

        TestQIStream stream(data);
        BinaryReader reader(stream);

        REQUIRE_THROWS_AS(readString(reader), std::runtime_error);
    }
}
