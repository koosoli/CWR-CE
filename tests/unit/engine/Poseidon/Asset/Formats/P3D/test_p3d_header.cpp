// test_p3d_header.cpp - Tests for P3D header reading
// Phase 1: File Header (12 bytes)

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Asset/Formats/P3D/P3DStructures.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include "../../../test_fixtures.hpp"
#include <fstream>
#include <vector>
#include <stdint.h>
#include <catch2/catch_message.hpp>
#include <cstring>
#include <stdexcept>
#include <string>

// Helper to load P3D file into memory
static std::vector<char> loadFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
    {
        throw std::runtime_error("Failed to open file: " + path);
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(static_cast<size_t>(size));
    file.read(buffer.data(), size);
    return buffer;
}

TEST_CASE("P3D Header: empty_shape.p3d", "[p3d][header][phase1]")
{
    auto data = loadFile(GET_FIXTURE("p3d/empty_shape.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    Poseidon::Asset::Formats::P3D::P3DHeader header = Poseidon::Asset::Formats::P3D::readHeader(reader);

    SECTION("Signature")
    {
        REQUIRE(std::memcmp(header.signature, "ODOL", 4) == 0);
        REQUIRE(header.isValid());
    }

    SECTION("Version")
    {
        REQUIRE(header.version == 7);
    }

    SECTION("LOD Count")
    {
        REQUIRE(header.lodCount == 1);
    }
}

TEST_CASE("P3D Header: sky_plane.p3d", "[p3d][header][phase1]")
{
    auto data = loadFile(GET_FIXTURE("p3d/sky_plane.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    Poseidon::Asset::Formats::P3D::P3DHeader header = Poseidon::Asset::Formats::P3D::readHeader(reader);

    REQUIRE(std::memcmp(header.signature, "ODOL", 4) == 0);
    REQUIRE(header.version == 7);
    REQUIRE(header.lodCount == 1);
    REQUIRE(header.isValid());
}

TEST_CASE("P3D Header: complex_vehicle.p3d", "[p3d][header][phase1]")
{
    auto data = loadFile(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    Poseidon::Asset::Formats::P3D::P3DHeader header = Poseidon::Asset::Formats::P3D::readHeader(reader);

    REQUIRE(std::memcmp(header.signature, "ODOL", 4) == 0);
    REQUIRE(header.version == 7);
    REQUIRE(header.lodCount > 1);   // Multi-LOD model
    REQUIRE(header.lodCount < 100); // Sanity check
    REQUIRE(header.isValid());
}

TEST_CASE("P3D Header: proxy_structure.p3d", "[p3d][header][phase1]")
{
    auto data = loadFile(GET_FIXTURE("p3d/proxy_structure.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    Poseidon::Asset::Formats::P3D::P3DHeader header = Poseidon::Asset::Formats::P3D::readHeader(reader);

    REQUIRE(std::memcmp(header.signature, "ODOL", 4) == 0);
    REQUIRE(header.version == 7);
    REQUIRE(header.lodCount >= 1);
    REQUIRE(header.isValid());
}

TEST_CASE("P3D Header: crew_proxy.p3d", "[p3d][header][phase1]")
{
    auto data = loadFile(GET_FIXTURE("p3d/crew_proxy.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    Poseidon::Asset::Formats::P3D::P3DHeader header = Poseidon::Asset::Formats::P3D::readHeader(reader);

    REQUIRE(std::memcmp(header.signature, "ODOL", 4) == 0);
    REQUIRE(header.version == 7);
    REQUIRE(header.lodCount >= 1);
    REQUIRE(header.isValid());
}

TEST_CASE("P3D Header: All fixtures catalog", "[p3d][header][phase1][catalog]")
{
    struct FixtureInfo
    {
        std::string path;
        uint32_t expectedVersion;
        uint32_t minLODs;
        bool isODOL; // Skip MLOD files for now
    };

    std::vector<FixtureInfo> fixtures = {
        {GET_FIXTURE("p3d/empty_shape.p3d"), 7, 1, true},       {GET_FIXTURE("p3d/sky_plane.p3d"), 7, 1, true},
        {GET_FIXTURE("p3d/complex_vehicle.p3d"), 7, 1, true},     {GET_FIXTURE("p3d/proxy_structure.p3d"), 7, 1, true},
        {GET_FIXTURE("p3d/crew_proxy.p3d"), 7, 1, true},
        // Skip MLOD files for Phase 1 (ODOL v7 only)
        // {"PoseidonTests/BinaryFormats/fixtures/complex_vehicle_mlod.p3d", 257, 1, false},
        // {"PoseidonTests/BinaryFormats/fixtures/camera_path_a.p3d", 257, 1, false},
        // {"PoseidonTests/BinaryFormats/fixtures/camera_path_b.p3d", 257, 1, false},
        // {"PoseidonTests/BinaryFormats/fixtures/camera_path_c.p3d", 257, 1, false},
    };

    for (const auto& fixture : fixtures)
    {
        if (!fixture.isODOL)
            continue; // Skip non-ODOL for now

        INFO("Testing: " << fixture.path);

        auto data = loadFile(fixture.path);
        QIStream stream(data.data(), static_cast<int>(data.size()));
        Poseidon::Asset::Formats::BinaryReader reader(stream);

        Poseidon::Asset::Formats::P3D::P3DHeader header = Poseidon::Asset::Formats::P3D::readHeader(reader);

        REQUIRE(std::memcmp(header.signature, "ODOL", 4) == 0);
        REQUIRE(header.version == fixture.expectedVersion);
        REQUIRE(header.lodCount >= fixture.minLODs);
        REQUIRE(header.lodCount < 100);
        REQUIRE(header.isValid());
    }
}
