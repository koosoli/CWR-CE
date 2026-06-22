#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <cstring>
#include "test_fixtures.hpp"
#include <stdint.h>
#include <utility>

// PAA format magic constants (from pactext.cpp)
static const uint16_t PAA_AI88 = 0x8080;
static const uint16_t PAA_ARGB4444 = 0x4444;
static const uint16_t PAA_ARGB1555 = 0x1555;
static const uint16_t PAA_DXT1 = 0xFF01;
static const uint16_t PAA_DXT2 = 0xFF02;
static const uint16_t PAA_DXT3 = 0xFF03;
static const uint16_t PAA_DXT4 = 0xFF04;
static const uint16_t PAA_DXT5 = 0xFF05;
// No decode fixtures available for: ARGB1555, DXT2, DXT4

static const uint32_t TAGG_MAGIC = 0x54414747; // "GGAT" LE = "TAGG"

static uint16_t readU16(std::ifstream& f)
{
    uint16_t v = 0;
    f.read(reinterpret_cast<char*>(&v), 2);
    return v;
}

static uint32_t readU32(std::ifstream& f)
{
    uint32_t v = 0;
    f.read(reinterpret_cast<char*>(&v), 4);
    return v;
}

// Skip TAGG sections and palette, return (width, height) of first mipmap
static std::pair<int, int> readPaaDimensions(std::ifstream& f)
{
    // Skip TAGG sections
    while (f.good())
    {
        uint32_t magic = readU32(f);
        if (magic != TAGG_MAGIC)
        {
            f.seekg(-4, std::ios::cur);
            break;
        }
        readU32(f); // tagg type
        uint32_t size = readU32(f);
        f.seekg(size, std::ios::cur);
    }
    // Palette: nColors
    uint16_t nColors = readU16(f);
    if (nColors > 0)
        f.seekg(nColors * 3, std::ios::cur);
    // First mipmap: width, height
    uint16_t w = readU16(f);
    uint16_t h = readU16(f);
    return {w, h};
}

TEST_CASE("PAA: synthetic_ai88.paa carries AI88 header metadata", "[Formats][PAA]")
{
    std::ifstream file(GET_FIXTURE("texture/paa/synthetic_ai88.paa"), std::ios::binary);
    REQUIRE(file.good());

    uint16_t magic = readU16(file);
    REQUIRE(magic == PAA_AI88);

    auto [w, h] = readPaaDimensions(file);
    REQUIRE(w == 64);
    REQUIRE(h == 64);
}

TEST_CASE("PAA: synthetic_argb4444.paa carries ARGB4444 header metadata", "[Formats][PAA]")
{
    std::ifstream file(GET_FIXTURE("texture/paa/synthetic_argb4444.paa"), std::ios::binary);
    REQUIRE(file.good());

    uint16_t magic = readU16(file);
    REQUIRE(magic == PAA_ARGB4444);

    auto [w, h] = readPaaDimensions(file);
    REQUIRE(w == 32);
    REQUIRE(h == 32);
}

TEST_CASE("PAA: synthetic_dxt1.paa is DXT1 format 64x64", "[Formats][PAA]")
{
    std::ifstream file(GET_FIXTURE("texture/paa/synthetic_dxt1.paa"), std::ios::binary);
    REQUIRE(file.good());

    uint16_t magic = readU16(file);
    REQUIRE(magic == PAA_DXT1);

    auto [w, h] = readPaaDimensions(file);
    REQUIRE(w == 64);
    REQUIRE(h == 64);
}

TEST_CASE("PAA: synthetic_dxt5.paa is DXT5 format 32x32", "[Formats][PAA]")
{
    std::ifstream file(GET_FIXTURE("paa/synthetic_dxt5.paa"), std::ios::binary);
    REQUIRE(file.good());

    uint16_t magic = readU16(file);
    REQUIRE(magic == PAA_DXT5);

    auto [w, h] = readPaaDimensions(file);
    REQUIRE(w == 32);
    REQUIRE(h == 32);
}
