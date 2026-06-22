#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include "test_helpers.hpp"
#include <stdint.h>
#include <vector>

using namespace Poseidon::Asset::Formats;
using Catch::Approx;

TEST_CASE("BISBinaryStream: Read primitive types", "[bis][framework][primitives]")
{
    std::vector<uint8_t> data;

    // Write test data
    writeValue(data, static_cast<uint8_t>(42));
    writeValue(data, static_cast<uint16_t>(1234));
    writeValue(data, static_cast<uint32_t>(567890));
    writeValue(data, static_cast<int8_t>(-42));
    writeValue(data, static_cast<int16_t>(-1234));
    writeValue(data, static_cast<int32_t>(-567890));
    writeValue(data, 3.14159f);

    TestQIStream stream(data);
    BinaryReader reader(stream);

    SECTION("Read uint8")
    {
        uint8_t value = reader.read<uint8_t>();
        REQUIRE(value == 42);
    }

    SECTION("Read uint16")
    {
        reader.read<uint8_t>(); // Skip first byte
        uint16_t value = reader.read<uint16_t>();
        REQUIRE(value == 1234);
    }

    SECTION("Read uint32")
    {
        reader.read<uint8_t>();
        reader.read<uint16_t>();
        uint32_t value = reader.read<uint32_t>();
        REQUIRE(value == 567890);
    }

    SECTION("Read signed integers")
    {
        reader.read<uint8_t>();
        reader.read<uint16_t>();
        reader.read<uint32_t>();

        int8_t i8 = reader.read<int8_t>();
        int16_t i16 = reader.read<int16_t>();
        int32_t i32 = reader.read<int32_t>();

        REQUIRE(i8 == -42);
        REQUIRE(i16 == -1234);
        REQUIRE(i32 == -567890);
    }

    SECTION("Read float")
    {
        // Skip: uint8(1) + uint16(2) + uint32(4) + int8(1) + int16(2) + int32(4) = 14 bytes
        reader.read<uint8_t>();  // 1
        reader.read<uint16_t>(); // 2
        reader.read<uint32_t>(); // 4
        reader.read<int8_t>();   // 1
        reader.read<int16_t>();  // 2
        reader.read<int32_t>();  // 4

        float value = reader.read<float>();
        REQUIRE(value == Approx(3.14159f));
    }
}
