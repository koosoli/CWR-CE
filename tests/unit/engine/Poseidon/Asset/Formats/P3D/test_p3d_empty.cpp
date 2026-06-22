// test_p3d_empty.cpp - Incremental tests for empty_shape.p3d
// Phase-by-phase parsing validation

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Asset/Formats/P3D/P3DStructures.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include "../../../test_fixtures.hpp"
#include <fstream>
#include <vector>
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

TEST_CASE("empty_shape.p3d: Phase 1 - Header", "[p3d][empty][phase1]")
{
    auto data = loadFile(GET_FIXTURE("p3d/empty_shape.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    auto header = Poseidon::Asset::Formats::P3D::readHeader(reader);

    REQUIRE(std::memcmp(header.signature, "ODOL", 4) == 0);
    REQUIRE(header.version == 7);
    REQUIRE(header.lodCount == 1);
    REQUIRE(header.isValid());
}

TEST_CASE("empty_shape.p3d: Phase 2 - Vertex Table", "[p3d][empty][phase2]")
{
    auto data = loadFile(GET_FIXTURE("p3d/empty_shape.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read header
    auto header = Poseidon::Asset::Formats::P3D::readHeader(reader);
    REQUIRE(header.lodCount == 1);

    // Read vertex table for LOD 0
    auto vtable = Poseidon::Asset::Formats::P3D::readVertexTable(reader);

    // empty_shape.p3d has no geometry
    REQUIRE(vtable.clipFlags.empty());
    REQUIRE(vtable.uvCoords.empty());
    REQUIRE(vtable.points.empty());
    REQUIRE(vtable.normals.empty());
}

TEST_CASE("empty_shape.p3d: Phase 3 - Bounding Volumes & Textures", "[p3d][empty][phase3]")
{
    auto data = loadFile(GET_FIXTURE("p3d/empty_shape.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read header
    auto header = Poseidon::Asset::Formats::P3D::readHeader(reader);
    REQUIRE(header.lodCount == 1);

    // Read vertex table
    auto vtable = Poseidon::Asset::Formats::P3D::readVertexTable(reader);

    // Read LOD bounds
    auto bounds = Poseidon::Asset::Formats::P3D::readLODBounds(reader);

    // Hardcoded expected values for empty_shape.p3d
    REQUIRE(bounds.orHints == 0);
    REQUIRE(bounds.andHints == 268435200);
    REQUIRE(bounds.minPos.x == 0.0f);
    REQUIRE(bounds.minPos.y == 0.0f);
    REQUIRE(bounds.minPos.z == 0.0f);
    REQUIRE(bounds.maxPos.x == 0.0f);
    REQUIRE(bounds.maxPos.y == 0.0f);
    REQUIRE(bounds.maxPos.z == 0.0f);
    REQUIRE(bounds.bCenter.x == 0.0f);
    REQUIRE(bounds.bCenter.y == 0.0f);
    REQUIRE(bounds.bCenter.z == 0.0f);
    REQUIRE(bounds.bRadius == 0.0f);

    // Read textures
    auto textures = Poseidon::Asset::Formats::P3D::readTextures(reader);

    // Empty model has no textures
    REQUIRE(textures.texturePaths.size() == 0);
}

TEST_CASE("empty_shape.p3d: Phase 4 - LOD Edges, Faces, Sections", "[p3d][empty][phase4]")
{
    auto data = loadFile(GET_FIXTURE("p3d/empty_shape.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read up to Phase 4
    auto header = Poseidon::Asset::Formats::P3D::readHeader(reader);
    auto vtable = Poseidon::Asset::Formats::P3D::readVertexTable(reader);
    auto bounds = Poseidon::Asset::Formats::P3D::readLODBounds(reader);
    auto textures = Poseidon::Asset::Formats::P3D::readTextures(reader);

    // Phase 4: Edges (empty)
    auto edges = Poseidon::Asset::Formats::P3D::readLODEdges(reader);
    REQUIRE(edges.mlodIndex.edges.size() == 0);
    REQUIRE(edges.vertexIndex.edges.size() == 0);

    // Phase 4: Faces (empty)
    auto faces = Poseidon::Asset::Formats::P3D::readFaces(reader);
    REQUIRE(faces.count == 0);
    REQUIRE(faces.faces.size() == 0);

    // Phase 4: Sections (empty)
    auto sections = Poseidon::Asset::Formats::P3D::readSections(reader);
    REQUIRE(sections.sections.size() == 0);
}

TEST_CASE("empty_shape.p3d: Phase 5 - Named Selections", "[p3d][empty][phase5]")
{
    auto data = loadFile(GET_FIXTURE("p3d/empty_shape.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read up to Phase 5
    auto header = Poseidon::Asset::Formats::P3D::readHeader(reader);
    auto vtable = Poseidon::Asset::Formats::P3D::readVertexTable(reader);
    auto bounds = Poseidon::Asset::Formats::P3D::readLODBounds(reader);
    auto textures = Poseidon::Asset::Formats::P3D::readTextures(reader);
    auto edges = Poseidon::Asset::Formats::P3D::readLODEdges(reader);
    auto faces = Poseidon::Asset::Formats::P3D::readFaces(reader);
    auto sections = Poseidon::Asset::Formats::P3D::readSections(reader);

    // Phase 5: Named Selections (empty)
    auto namedSelections = Poseidon::Asset::Formats::P3D::readNamedSelections(reader);
    REQUIRE(namedSelections.selections.size() == 0);
}
