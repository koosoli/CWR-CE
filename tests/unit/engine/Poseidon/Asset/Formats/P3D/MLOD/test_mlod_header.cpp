// test_mlod_header.cpp - Tests for MLOD header reading
// Batch 1: File Header (16 bytes)

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Asset/Formats/P3D/MLODStructures.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include "../../../../test_fixtures.hpp"
#include <fstream>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <string>

// Helper to load MLOD file into memory
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

TEST_CASE("MLOD Header: complex_vehicle_mlod.p3d", "[mlod][header][batch1]")
{
    auto data = loadFile(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    auto header = Poseidon::Asset::Formats::MLOD::readHeader(reader);

    SECTION("Signature")
    {
        REQUIRE(std::memcmp(header.signature, "MLOD", 4) == 0);
        REQUIRE(header.isValid());
        REQUIRE(header.isMLOD());
    }

    SECTION("Version")
    {
        REQUIRE(header.versionMajor == 1);
        REQUIRE(header.versionMinor == 1);
    }

    SECTION("Padding")
    {
        REQUIRE(header.padding == 0);
    }

    SECTION("LOD Count")
    {
        REQUIRE(header.lodCount == 19);
    }
}

TEST_CASE("MLOD Header: complex_vehicle_mlod.p3d repeat read", "[mlod][header][batch1]")
{
    auto data = loadFile(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    auto header = Poseidon::Asset::Formats::MLOD::readHeader(reader);

    REQUIRE(std::memcmp(header.signature, "MLOD", 4) == 0);
    REQUIRE(header.isValid());
    REQUIRE(header.versionMajor == 1);
    REQUIRE(header.versionMinor == 1);
    REQUIRE(header.lodCount >= 1);
    REQUIRE(header.lodCount <= 100);
}

TEST_CASE("MLOD Header: camera_path_a.p3d", "[mlod][header][batch1]")
{
    auto data = loadFile(GET_FIXTURE("mlod/camera_path_a.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    auto header = Poseidon::Asset::Formats::MLOD::readHeader(reader);

    REQUIRE(std::memcmp(header.signature, "MLOD", 4) == 0);
    REQUIRE(header.isValid());
    REQUIRE(header.versionMajor >= 1);
    REQUIRE(header.lodCount >= 1);
}

TEST_CASE("MLOD Header: camera_path_b.p3d", "[mlod][header][batch1]")
{
    auto data = loadFile(GET_FIXTURE("mlod/camera_path_b.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    auto header = Poseidon::Asset::Formats::MLOD::readHeader(reader);

    REQUIRE(std::memcmp(header.signature, "MLOD", 4) == 0);
    REQUIRE(header.isValid());
    REQUIRE(header.versionMajor >= 1);
    REQUIRE(header.lodCount >= 1);
}

TEST_CASE("MLOD Header: camera_path_c.p3d", "[mlod][header][batch1]")
{
    auto data = loadFile(GET_FIXTURE("mlod/camera_path_c.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    auto header = Poseidon::Asset::Formats::MLOD::readHeader(reader);

    REQUIRE(std::memcmp(header.signature, "MLOD", 4) == 0);
    REQUIRE(header.isValid());
    REQUIRE(header.versionMajor >= 1);
    REQUIRE(header.lodCount >= 1);
}
