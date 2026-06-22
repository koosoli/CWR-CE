#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include "test_helpers.hpp"
#include <stdint.h>
#include <string>
#include <vector>

using namespace Poseidon::Asset::Formats;

TEST_CASE("BISBinaryStream: Read strings", "[bis][framework][strings]")
{
    SECTION("Read ASCIIZ string")
    {
        std::vector<uint8_t> data;
        writeAsciiz(data, "Hello World");

        TestQIStream stream(data);
        BinaryReader reader(stream);

        std::string value = readAsciiz(reader);
        REQUIRE(value == "Hello World");
    }

    SECTION("Read empty ASCIIZ string")
    {
        std::vector<uint8_t> data;
        writeAsciiz(data, "");

        TestQIStream stream(data);
        BinaryReader reader(stream);

        std::string value = readAsciiz(reader);
        REQUIRE(value == "");
    }

    SECTION("Read length-prefixed string")
    {
        std::vector<uint8_t> data;
        writeString(data, "Test String");

        TestQIStream stream(data);
        BinaryReader reader(stream);

        std::string value = readString(reader);
        REQUIRE(value == "Test String");
    }

    SECTION("Read empty length-prefixed string")
    {
        std::vector<uint8_t> data;
        writeString(data, "");

        TestQIStream stream(data);
        BinaryReader reader(stream);

        std::string value = readString(reader);
        REQUIRE(value == "");
    }
}
