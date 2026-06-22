#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <Poseidon/Asset/Formats/BISStructures.hpp>
#include "test_helpers.hpp"
#include <stdint.h>
#include <vector>

using namespace Poseidon::Asset::Formats;
using Catch::Approx;
namespace bis = Poseidon::Asset::Formats; // qualify Vector3 vs the prelude's Foundation::Vector3P

TEST_CASE("BISBinaryStream: Read structures", "[bis][framework][structures]")
{
    SECTION("Read Vector2")
    {
        std::vector<uint8_t> data;
        writeValue(data, 1.5f);
        writeValue(data, 2.5f);

        TestQIStream stream(data);
        BinaryReader reader(stream);

        Vector2 vec;
        read(reader, vec);

        REQUIRE(vec.u == Approx(1.5f));
        REQUIRE(vec.v == Approx(2.5f));
    }

    SECTION("Read Vector3")
    {
        std::vector<uint8_t> data;
        writeValue(data, 1.0f);
        writeValue(data, 2.0f);
        writeValue(data, 3.0f);

        TestQIStream stream(data);
        BinaryReader reader(stream);

        bis::Vector3 vec;
        read(reader, vec);

        REQUIRE(vec.x == Approx(1.0f));
        REQUIRE(vec.y == Approx(2.0f));
        REQUIRE(vec.z == Approx(3.0f));
    }

    SECTION("Read BoundingBox")
    {
        std::vector<uint8_t> data;
        writeValue(data, -10.0f);
        writeValue(data, -20.0f);
        writeValue(data, -30.0f);
        writeValue(data, 10.0f);
        writeValue(data, 20.0f);
        writeValue(data, 30.0f);

        TestQIStream stream(data);
        BinaryReader reader(stream);

        BoundingBox bbox;
        read(reader, bbox);

        REQUIRE(bbox.min.x == Approx(-10.0f));
        REQUIRE(bbox.min.y == Approx(-20.0f));
        REQUIRE(bbox.min.z == Approx(-30.0f));
        REQUIRE(bbox.max.x == Approx(10.0f));
        REQUIRE(bbox.max.y == Approx(20.0f));
        REQUIRE(bbox.max.z == Approx(30.0f));
    }

    SECTION("Read BoundingSphere")
    {
        std::vector<uint8_t> data;
        writeValue(data, 5.0f);  // center.x
        writeValue(data, 10.0f); // center.y
        writeValue(data, 15.0f); // center.z
        writeValue(data, 25.0f); // radius

        TestQIStream stream(data);
        BinaryReader reader(stream);

        BoundingSphere sphere;
        read(reader, sphere);

        REQUIRE(sphere.center.x == Approx(5.0f));
        REQUIRE(sphere.center.y == Approx(10.0f));
        REQUIRE(sphere.center.z == Approx(15.0f));
        REQUIRE(sphere.radius == Approx(25.0f));
    }

    SECTION("Read ColorBGRA")
    {
        std::vector<uint8_t> data;
        uint32_t color = 0xFF8040C0; // A=255, R=128, G=64, B=192
        writeValue(data, color);

        TestQIStream stream(data);
        BinaryReader reader(stream);

        ColorBGRA col;
        read(reader, col);

        REQUIRE(col.b == 192);
        REQUIRE(col.g == 64);
        REQUIRE(col.r == 128);
        REQUIRE(col.a == 255);
    }
}
