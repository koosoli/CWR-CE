// test_p3d_sky_plane.cpp - Incremental tests for sky_plane.p3d
// Phase-by-phase parsing validation

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <Poseidon/Asset/Formats/P3D/P3DStructures.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include "../../../test_fixtures.hpp"
#include <fstream>
#include <vector>
#include <stdint.h>
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

TEST_CASE("sky_plane.p3d: Phase 1 - Header", "[p3d][sky_plane][phase1]")
{
    auto data = loadFile(GET_FIXTURE("p3d/sky_plane.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    auto header = Poseidon::Asset::Formats::P3D::readHeader(reader);

    REQUIRE(std::memcmp(header.signature, "ODOL", 4) == 0);
    REQUIRE(header.version == 7);
    REQUIRE(header.lodCount == 1);
    REQUIRE(header.isValid());
}

TEST_CASE("sky_plane.p3d: Phase 2 - Vertex Table", "[p3d][sky_plane][phase2]")
{
    auto data = loadFile(GET_FIXTURE("p3d/sky_plane.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read header
    auto header = Poseidon::Asset::Formats::P3D::readHeader(reader);
    REQUIRE(header.lodCount == 1);

    // Read vertex table for LOD 0
    auto vtable = Poseidon::Asset::Formats::P3D::readVertexTable(reader);

    // Hardcoded array sizes
    REQUIRE(vtable.clipFlags.size() == 53);
    REQUIRE(vtable.uvCoords.size() == 53);
    REQUIRE(vtable.points.size() == 53);
    REQUIRE(vtable.normals.size() == 53);

    // Hardcoded sample values: first, last, middle
    REQUIRE(vtable.points[0].x == Catch::Approx(0.0264602f));
    REQUIRE(vtable.points[0].y == Catch::Approx(7.09607f));
    REQUIRE(vtable.points[0].z == Catch::Approx(-0.133074f));

    REQUIRE(vtable.points[52].x == Catch::Approx(5.40684f));
    REQUIRE(vtable.points[52].y == Catch::Approx(-7.09607f));
    REQUIRE(vtable.points[52].z == Catch::Approx(8.25811f));

    REQUIRE(vtable.points[5].x == Catch::Approx(-4.99699f));
    REQUIRE(vtable.points[5].y == Catch::Approx(5.47451f));
    REQUIRE(vtable.points[5].z == Catch::Approx(2.16106f));

    // UV coords sample
    REQUIRE(vtable.uvCoords[0].u == Catch::Approx(0.0407328f));
    REQUIRE(vtable.uvCoords[0].v == Catch::Approx(0.0357998f));
    REQUIRE(vtable.uvCoords[52].u == Catch::Approx(0.178741f));
    REQUIRE(vtable.uvCoords[52].v == Catch::Approx(1.0f));
}

TEST_CASE("sky_plane.p3d: Phase 3 - Bounding Volumes & Textures", "[p3d][sky_plane][phase3]")
{
    auto data = loadFile(GET_FIXTURE("p3d/sky_plane.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read header
    auto header = Poseidon::Asset::Formats::P3D::readHeader(reader);
    REQUIRE(header.lodCount == 1);

    // Read vertex table
    auto vtable = Poseidon::Asset::Formats::P3D::readVertexTable(reader);

    // Read LOD bounds
    auto bounds = Poseidon::Asset::Formats::P3D::readLODBounds(reader);

    // Hardcoded bounds values
    REQUIRE(bounds.orHints == 16384);
    REQUIRE(bounds.andHints == 16384);
    REQUIRE(bounds.minPos.x == Catch::Approx(-9.91259f));
    REQUIRE(bounds.minPos.y == Catch::Approx(-7.09607f));
    REQUIRE(bounds.minPos.z == Catch::Approx(-9.84711f));
    REQUIRE(bounds.maxPos.x == Catch::Approx(9.91259f));
    REQUIRE(bounds.maxPos.y == Catch::Approx(7.09607f));
    REQUIRE(bounds.maxPos.z == Catch::Approx(9.84711f));
    REQUIRE(bounds.bCenter.x == Catch::Approx(0.0f));
    REQUIRE(bounds.bCenter.y == Catch::Approx(0.0f));
    REQUIRE(bounds.bCenter.z == Catch::Approx(0.0f));
    REQUIRE(bounds.bRadius == Catch::Approx(12.3986f));

    // Read textures
    auto textures = Poseidon::Asset::Formats::P3D::readTextures(reader);

    // Hardcoded texture list
    REQUIRE(textures.texturePaths.size() == 1);
    REQUIRE(textures.texturePaths[0] == "data\\sky_plane.pac");
}

TEST_CASE("sky_plane.p3d: Phase 4 - LOD Edges, Faces, Sections", "[p3d][sky_plane][phase4]")
{
    auto data = loadFile(GET_FIXTURE("p3d/sky_plane.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read up to Phase 4
    auto header = Poseidon::Asset::Formats::P3D::readHeader(reader);
    auto vtable = Poseidon::Asset::Formats::P3D::readVertexTable(reader);
    auto bounds = Poseidon::Asset::Formats::P3D::readLODBounds(reader);
    auto textures = Poseidon::Asset::Formats::P3D::readTextures(reader);

    // Phase 4: Edges
    auto edges = Poseidon::Asset::Formats::P3D::readLODEdges(reader);
    REQUIRE(edges.mlodIndex.edges.size() == 45);
    REQUIRE(edges.vertexIndex.edges.size() == 53);

    // Phase 4: Faces
    auto faces = Poseidon::Asset::Formats::P3D::readFaces(reader);
    REQUIRE(faces.count == 77);
    REQUIRE(faces.offsetToSections == 1232);
    REQUIRE(faces.faces.size() == 77);

    // Check first face
    const auto& f0 = faces.faces[0];
    REQUIRE(f0.flags == 32768);
    REQUIRE(f0.textureIndex == 0);
    REQUIRE(f0.vertexCount == 3);
    REQUIRE(f0.vertexIndices.size() == 3);

    // Validate texture indices in range
    for (const auto& face : faces.faces)
    {
        if (face.textureIndex >= 0)
        {
            REQUIRE(face.textureIndex < static_cast<int16_t>(textures.texturePaths.size()));
        }
    }

    // Phase 4: Sections
    auto sections = Poseidon::Asset::Formats::P3D::readSections(reader);
    REQUIRE(sections.sections.size() == 1);

    const auto& s0 = sections.sections[0];
    REQUIRE(s0.faceIndexLowerBound == 0);
    REQUIRE(s0.faceIndexUpperBound == 1232);
    REQUIRE(s0.material == 0);
    REQUIRE(s0.textureIndex == 0);
    REQUIRE(s0.special == 32768);

    // Validate section texture indices
    for (const auto& section : sections.sections)
    {
        if (section.textureIndex >= 0)
        {
            REQUIRE(section.textureIndex < static_cast<int16_t>(textures.texturePaths.size()));
        }
    }
}

TEST_CASE("sky_plane.p3d: Phase 5 - Named Selections", "[p3d][sky_plane][phase5]")
{
    auto data = loadFile(GET_FIXTURE("p3d/sky_plane.p3d"));
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

    // Phase 5: Named Selections
    auto namedSelections = Poseidon::Asset::Formats::P3D::readNamedSelections(reader);

    // sky_plane has no named selections
    REQUIRE(namedSelections.selections.size() == 0);
}

TEST_CASE("sky_plane.p3d: Phase 6 - Named Properties", "[p3d][sky_plane][phase6]")
{
    auto data = loadFile(GET_FIXTURE("p3d/sky_plane.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read up to Phase 6
    auto header = Poseidon::Asset::Formats::P3D::readHeader(reader);
    auto vtable = Poseidon::Asset::Formats::P3D::readVertexTable(reader);
    auto bounds = Poseidon::Asset::Formats::P3D::readLODBounds(reader);
    auto textures = Poseidon::Asset::Formats::P3D::readTextures(reader);
    auto edges = Poseidon::Asset::Formats::P3D::readLODEdges(reader);
    auto faces = Poseidon::Asset::Formats::P3D::readFaces(reader);
    auto sections = Poseidon::Asset::Formats::P3D::readSections(reader);
    auto namedSelections = Poseidon::Asset::Formats::P3D::readNamedSelections(reader);

    // Phase 6: Named Properties
    auto namedProperties = Poseidon::Asset::Formats::P3D::readNamedProperties(reader);

    // sky_plane has no named properties
    REQUIRE(namedProperties.properties.size() == 0);
}

TEST_CASE("sky_plane.p3d: Phase 7 - Frames", "[p3d][sky_plane][phase7]")
{
    auto data = loadFile(GET_FIXTURE("p3d/sky_plane.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read up to Phase 7
    auto header = Poseidon::Asset::Formats::P3D::readHeader(reader);
    auto vtable = Poseidon::Asset::Formats::P3D::readVertexTable(reader);
    auto bounds = Poseidon::Asset::Formats::P3D::readLODBounds(reader);
    auto textures = Poseidon::Asset::Formats::P3D::readTextures(reader);
    auto edges = Poseidon::Asset::Formats::P3D::readLODEdges(reader);
    auto faces = Poseidon::Asset::Formats::P3D::readFaces(reader);
    auto sections = Poseidon::Asset::Formats::P3D::readSections(reader);
    auto namedSelections = Poseidon::Asset::Formats::P3D::readNamedSelections(reader);
    auto namedProperties = Poseidon::Asset::Formats::P3D::readNamedProperties(reader);

    // Phase 7: Frames
    auto frames = Poseidon::Asset::Formats::P3D::readFrames(reader);

    // sky_plane has no animation frames
    REQUIRE(frames.frames.size() == 0);
}

TEST_CASE("sky_plane.p3d: Phase 8 - LOD End Data", "[p3d][sky_plane][phase8]")
{
    auto data = loadFile(GET_FIXTURE("p3d/sky_plane.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read up to Phase 8
    auto header = Poseidon::Asset::Formats::P3D::readHeader(reader);
    auto vtable = Poseidon::Asset::Formats::P3D::readVertexTable(reader);
    auto bounds = Poseidon::Asset::Formats::P3D::readLODBounds(reader);
    auto textures = Poseidon::Asset::Formats::P3D::readTextures(reader);
    auto edges = Poseidon::Asset::Formats::P3D::readLODEdges(reader);
    auto faces = Poseidon::Asset::Formats::P3D::readFaces(reader);
    auto sections = Poseidon::Asset::Formats::P3D::readSections(reader);
    auto namedSelections = Poseidon::Asset::Formats::P3D::readNamedSelections(reader);
    auto namedProperties = Poseidon::Asset::Formats::P3D::readNamedProperties(reader);
    auto frames = Poseidon::Asset::Formats::P3D::readFrames(reader);

    // Phase 8: LOD End Data
    auto lodEndData = Poseidon::Asset::Formats::P3D::readLODEndData(reader);

    // Hardcoded color values
    REQUIRE(lodEndData.colorTop == 4287137920);
    REQUIRE(lodEndData.color == 4287137920);
    REQUIRE(lodEndData.special == 0);
}

TEST_CASE("sky_plane.p3d: Phase 9 - Proxies", "[p3d][sky_plane][phase9]")
{
    auto data = loadFile(GET_FIXTURE("p3d/sky_plane.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read up to Phase 9 (complete LOD)
    auto header = Poseidon::Asset::Formats::P3D::readHeader(reader);
    auto vtable = Poseidon::Asset::Formats::P3D::readVertexTable(reader);
    auto bounds = Poseidon::Asset::Formats::P3D::readLODBounds(reader);
    auto textures = Poseidon::Asset::Formats::P3D::readTextures(reader);
    auto edges = Poseidon::Asset::Formats::P3D::readLODEdges(reader);
    auto faces = Poseidon::Asset::Formats::P3D::readFaces(reader);
    auto sections = Poseidon::Asset::Formats::P3D::readSections(reader);
    auto namedSelections = Poseidon::Asset::Formats::P3D::readNamedSelections(reader);
    auto namedProperties = Poseidon::Asset::Formats::P3D::readNamedProperties(reader);
    auto frames = Poseidon::Asset::Formats::P3D::readFrames(reader);
    auto lodEndData = Poseidon::Asset::Formats::P3D::readLODEndData(reader);

    // Phase 9: Proxies
    auto proxies = Poseidon::Asset::Formats::P3D::readProxies(reader);

    // sky_plane has no proxies
    REQUIRE(proxies.proxies.size() == 0);
}
