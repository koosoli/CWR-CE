#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <cstring>
#include "test_fixtures.hpp"
#include <stdint.h>

// PAA format codes that can appear in PAC files
static const uint16_t PAA_DXT1 = 0xFF01;
static const uint16_t PAA_DXT5 = 0xFF05;
// LZW compression markers (from pactext.cpp)
static const uint16_t MAGIC_W_LZW = 1234;
static const uint16_t MAGIC_H_LZW = 8765;

static bool isPaaFormat(uint16_t desc)
{
    return desc == 0x8080 || desc == 0x4444 || desc == 0x1555 || (desc >= 0xFF01 && desc <= 0xFF05);
}

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

static const uint32_t TAGG_MAGIC = 0x54414747;

struct PacHeader
{
    bool hasPaaDescriptor;
    uint16_t paaDescriptor;
    int paletteColors;
    int width;
    int height;
    bool lzwCompressed;
};

static PacHeader readPacHeader(std::ifstream& f)
{
    PacHeader h = {};
    // Try PAA format descriptor
    uint16_t desc = readU16(f);
    if (isPaaFormat(desc))
    {
        h.hasPaaDescriptor = true;
        h.paaDescriptor = desc;
    }
    else
    {
        f.seekg(-2, std::ios::cur);
    }
    // Skip TAGG sections
    while (f.good())
    {
        uint32_t magic = readU32(f);
        if (magic != TAGG_MAGIC)
        {
            f.seekg(-4, std::ios::cur);
            break;
        }
        readU32(f); // type
        uint32_t size = readU32(f);
        f.seekg(size, std::ios::cur);
    }
    // Palette
    h.paletteColors = readU16(f);
    if (h.paletteColors > 0)
        f.seekg(h.paletteColors * 3, std::ios::cur);
    // First mipmap dimensions
    uint16_t w = readU16(f);
    uint16_t ht = readU16(f);
    if (w == MAGIC_W_LZW && ht == MAGIC_H_LZW)
    {
        h.lzwCompressed = true;
        w = readU16(f);
        ht = readU16(f);
    }
    h.width = w;
    h.height = ht;
    return h;
}

TEST_CASE("PAC: synthetic_default.pac is DXT5 format 64x64", "[Formats][PAC]")
{
    std::ifstream file(GET_FIXTURE("texture/pac/synthetic_default.pac"), std::ios::binary);
    REQUIRE(file.good());

    auto h = readPacHeader(file);
    REQUIRE(h.hasPaaDescriptor);
    REQUIRE(h.paaDescriptor == PAA_DXT5);
    REQUIRE(h.paletteColors == 0);
    REQUIRE(h.width == 64);
    REQUIRE(h.height == 64);
    REQUIRE_FALSE(h.lzwCompressed);
}

TEST_CASE("PAC: synthetic_dark.pac is DXT5 format 64x64", "[Formats][PAC]")
{
    std::ifstream file(GET_FIXTURE("texture/pac/synthetic_dark.pac"), std::ios::binary);
    REQUIRE(file.good());

    auto h = readPacHeader(file);
    REQUIRE(h.hasPaaDescriptor);
    REQUIRE(h.paaDescriptor == PAA_DXT5);
    REQUIRE(h.paletteColors == 0);
    REQUIRE_FALSE(h.lzwCompressed);
    REQUIRE(h.width == 64);
    REQUIRE(h.height == 64);
}
