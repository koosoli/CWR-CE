#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include "test_helpers.hpp"
#include <stdint.h>
#include <vector>

using namespace Poseidon::Asset::Formats;

TEST_CASE("BISBinaryStream: Stream position", "[bis][framework][position]")
{
    std::vector<uint8_t> data;
    writeValue(data, static_cast<uint32_t>(42));
    writeValue(data, static_cast<uint16_t>(123));
    writeValue(data, static_cast<uint8_t>(7));

    TestQIStream stream(data);
    BinaryReader reader(stream);

    SECTION("Tell position")
    {
        REQUIRE(reader.tell() == 0);

        reader.read<uint32_t>();
        REQUIRE(reader.tell() == 4);

        reader.read<uint16_t>();
        REQUIRE(reader.tell() == 6);

        reader.read<uint8_t>();
        REQUIRE(reader.tell() == 7);
    }

    SECTION("Seek position")
    {
        reader.seek(4);
        REQUIRE(reader.tell() == 4);

        uint16_t value = reader.read<uint16_t>();
        REQUIRE(value == 123);
    }
}
