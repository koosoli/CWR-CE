// test_mlod_sp3x.cpp - Tests for MLOD SP3X section reading
// Batch 2: Vertex & Face Data

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Asset/Formats/P3D/MLODStructures.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include "../../../../test_fixtures.hpp"
#include <fstream>
#include <vector>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

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

TEST_CASE("MLOD SP3X: complex_vehicle_mlod.p3d - Header", "[mlod][sp3x][batch2]")
{
    auto data = loadFile(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read MLOD header
    auto header = Poseidon::Asset::Formats::MLOD::readHeader(reader);
    REQUIRE(header.lodCount == 19);

    // Read SP3X section for LOD 0
    auto sp3x = Poseidon::Asset::Formats::MLOD::readSP3XHeader(reader);

    SECTION("Signature")
    {
        REQUIRE(std::memcmp(sp3x.signature, "SP3X", 4) == 0);
        REQUIRE(sp3x.isValid());
    }

    SECTION("Header size")
    {
        REQUIRE(sp3x.headSize == 28);
    }

    SECTION("Counts")
    {
        REQUIRE(sp3x.nPos == 2617);
        REQUIRE(sp3x.nNorm == 4815);
        REQUIRE(sp3x.nFace == 1341);
    }
}

TEST_CASE("MLOD SP3X: camera_path_a.p3d - Simple model", "[mlod][sp3x][batch2]")
{
    auto data = loadFile(GET_FIXTURE("mlod/camera_path_a.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read MLOD header
    auto header = Poseidon::Asset::Formats::MLOD::readHeader(reader);
    REQUIRE(header.lodCount == 1);

    // Read SP3X section
    auto sp3x = Poseidon::Asset::Formats::MLOD::readSP3XHeader(reader);

    REQUIRE(std::memcmp(sp3x.signature, "SP3X", 4) == 0);
    REQUIRE(sp3x.headSize == 28);
    REQUIRE(sp3x.nPos == 2);
    REQUIRE(sp3x.nNorm == 0);
}

TEST_CASE("MLOD Vertices: complex_vehicle_mlod.p3d - Read positions", "[mlod][vertices][batch2]")
{
    auto data = loadFile(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read headers
    auto header = Poseidon::Asset::Formats::MLOD::readHeader(reader);
    auto sp3x = Poseidon::Asset::Formats::MLOD::readSP3XHeader(reader);

    // Read vertex positions
    auto vtable = Poseidon::Asset::Formats::MLOD::readVertexTable(reader, sp3x.nPos);

    SECTION("Vertex count")
    {
        REQUIRE(vtable.points.size() == 2617);
    }

    SECTION("Vertex positions are reasonable")
    {
        // Sample vertices (first, last, middle) instead of all 2617
        REQUIRE(std::abs(vtable.points[0].position.x) < 100.0f);
        REQUIRE(std::abs(vtable.points[0].position.y) < 100.0f);
        REQUIRE(std::abs(vtable.points[0].position.z) < 100.0f);

        REQUIRE(std::abs(vtable.points.back().position.x) < 100.0f);
        REQUIRE(std::abs(vtable.points.back().position.y) < 100.0f);
        REQUIRE(std::abs(vtable.points.back().position.z) < 100.0f);

        REQUIRE(std::abs(vtable.points[1308].position.x) < 100.0f);
        REQUIRE(std::abs(vtable.points[1308].position.y) < 100.0f);
        REQUIRE(std::abs(vtable.points[1308].position.z) < 100.0f);
    }
}

TEST_CASE("MLOD Normals: complex_vehicle_mlod.p3d - Read normals", "[mlod][normals][batch2]")
{
    auto data = loadFile(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read headers
    auto header = Poseidon::Asset::Formats::MLOD::readHeader(reader);
    auto sp3x = Poseidon::Asset::Formats::MLOD::readSP3XHeader(reader);

    // Read vertex positions
    auto vtable = Poseidon::Asset::Formats::MLOD::readVertexTable(reader, sp3x.nPos);

    // Read normals
    auto ntable = Poseidon::Asset::Formats::MLOD::readNormalTable(reader, sp3x.nNorm);

    SECTION("Normal count")
    {
        REQUIRE(ntable.normals.size() == 4815);
    }

    SECTION("Normals are finite values")
    {
        // Sample normals (first, last, middle) instead of all 4815
        REQUIRE(std::isfinite(ntable.normals[0].x));
        REQUIRE(std::isfinite(ntable.normals[0].y));
        REQUIRE(std::isfinite(ntable.normals[0].z));

        REQUIRE(std::isfinite(ntable.normals.back().x));
        REQUIRE(std::isfinite(ntable.normals.back().y));
        REQUIRE(std::isfinite(ntable.normals.back().z));

        REQUIRE(std::isfinite(ntable.normals[2407].x));
        REQUIRE(std::isfinite(ntable.normals[2407].y));
        REQUIRE(std::isfinite(ntable.normals[2407].z));
    }
}

TEST_CASE("MLOD Faces: camera_path_a.p3d - Stub for Batch 3", "[mlod][faces][batch3]")
{
    // Face reading will be implemented in Batch 3
    // camera_path_a is too small to verify full structure
    REQUIRE(true);
}
