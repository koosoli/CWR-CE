// test_mlod_parity.cpp - Comprehensive parity tests between MLOD and ODOL formats
// Compare complex_vehicle_mlod.p3d (source format) with complex_vehicle.p3d (compiled format)
// These tests verify that both formats represent the same 3D model data

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Asset/Formats/P3D/MLODStructures.hpp>
#include <Poseidon/Asset/Formats/P3D/P3DStructures.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include "../../../../test_fixtures.hpp"
#include <fstream>
#include <vector>
#include <catch2/catch_message.hpp>
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

// HEADER PARITY

TEST_CASE("Parity: complex_vehicle Header - MLOD vs ODOL", "[mlod][parity][header]")
{
    auto mlodData = loadFile(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));
    QIStream mlodStream(mlodData.data(), static_cast<int>(mlodData.size()));
    Poseidon::Asset::Formats::BinaryReader mlodReader(mlodStream);

    auto odolData = loadFile(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    QIStream odolStream(odolData.data(), static_cast<int>(odolData.size()));
    Poseidon::Asset::Formats::BinaryReader odolReader(odolStream);

    auto mlodHeader = Poseidon::Asset::Formats::MLOD::readHeader(mlodReader);
    auto odolHeader = Poseidon::Asset::Formats::P3D::readHeader(odolReader);

    // Signatures differ (MLOD vs ODOL)
    REQUIRE(std::memcmp(mlodHeader.signature, "MLOD", 4) == 0);
    REQUIRE(std::memcmp(odolHeader.signature, "ODOL", 4) == 0);

    // LOD count must match
    REQUIRE(mlodHeader.lodCount == 19);
    REQUIRE(odolHeader.lodCount == 19);
    REQUIRE(mlodHeader.lodCount == odolHeader.lodCount);
}

// GEOMETRY COUNTS PARITY

TEST_CASE("Parity: complex_vehicle LOD 0 Counts - MLOD vs ODOL", "[mlod][parity][counts]")
{
    auto mlodData = loadFile(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));
    QIStream mlodStream(mlodData.data(), static_cast<int>(mlodData.size()));
    Poseidon::Asset::Formats::BinaryReader mlodReader(mlodStream);

    auto odolData = loadFile(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    QIStream odolStream(odolData.data(), static_cast<int>(odolData.size()));
    Poseidon::Asset::Formats::BinaryReader odolReader(odolStream);

    auto mlodHeader = Poseidon::Asset::Formats::MLOD::readHeader(mlodReader);
    auto sp3x = Poseidon::Asset::Formats::MLOD::readSP3XHeader(mlodReader);

    auto odolHeader = Poseidon::Asset::Formats::P3D::readHeader(odolReader);
    auto vtable = Poseidon::Asset::Formats::P3D::readVertexTable(odolReader);

    INFO("MLOD SP3X nPos: " << sp3x.nPos);
    INFO("MLOD SP3X nNorm: " << sp3x.nNorm);
    INFO("MLOD SP3X nFace: " << sp3x.nFace);
    INFO("ODOL vertex count: " << vtable.points.size());

    // Both formats have same expanded vertex count
    REQUIRE(sp3x.nPos == 2617);
    REQUIRE(vtable.points.size() == 2617);
    REQUIRE(sp3x.nPos == vtable.points.size());

    // MLOD has more normals (shared normals vs per-vertex)
    REQUIRE(sp3x.nNorm == 4815);
    REQUIRE(vtable.normals.size() == 2617);

    // Face count matches
    REQUIRE(sp3x.nFace == 1341);
}

// VERTEX POSITIONS PARITY

TEST_CASE("Parity: complex_vehicle LOD 0 Vertices - MLOD positions vs ODOL", "[mlod][parity][vertices]")
{
    auto mlodData = loadFile(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));
    QIStream mlodStream(mlodData.data(), static_cast<int>(mlodData.size()));
    Poseidon::Asset::Formats::BinaryReader mlodReader(mlodStream);

    auto odolData = loadFile(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    QIStream odolStream(odolData.data(), static_cast<int>(odolData.size()));
    Poseidon::Asset::Formats::BinaryReader odolReader(odolStream);

    auto mlodHeader = Poseidon::Asset::Formats::MLOD::readHeader(mlodReader);
    auto sp3x = Poseidon::Asset::Formats::MLOD::readSP3XHeader(mlodReader);
    auto mlodVerts = Poseidon::Asset::Formats::MLOD::readVertexTable(mlodReader, sp3x.nPos);
    auto mlodNorms = Poseidon::Asset::Formats::MLOD::readNormalTable(mlodReader, sp3x.nNorm);

    auto odolHeader = Poseidon::Asset::Formats::P3D::readHeader(odolReader);
    auto odolVerts = Poseidon::Asset::Formats::P3D::readVertexTable(odolReader);

    // Vertex counts match
    REQUIRE(mlodVerts.points.size() == 2617);
    REQUIRE(odolVerts.points.size() == 2617);
    REQUIRE(mlodVerts.points.size() == odolVerts.points.size());

    // MLOD normal count (shared normals)
    REQUIRE(mlodNorms.normals.size() == 4815);

    // Check specific MLOD vertex positions
    REQUIRE(mlodVerts.points[0].position.x == Catch::Approx(0.125648f));
    REQUIRE(mlodVerts.points[0].position.y == Catch::Approx(2.3094f));
    REQUIRE(mlodVerts.points[0].position.z == Catch::Approx(6.359147f));

    REQUIRE(mlodVerts.points[2616].position.x == Catch::Approx(0.005223222f));
    REQUIRE(mlodVerts.points[2616].position.y == Catch::Approx(-1.99179f));
    REQUIRE(mlodVerts.points[2616].position.z == Catch::Approx(-3.143456f));

    REQUIRE(mlodVerts.points[1308].position.x == Catch::Approx(0.8838605f));
    REQUIRE(mlodVerts.points[1308].position.y == Catch::Approx(-1.24811f));

    // ODOL vertex positions (different due to compression/expansion)
    REQUIRE(odolVerts.points[0].x == Catch::Approx(0.205741f));
    REQUIRE(odolVerts.points[0].y == Catch::Approx(2.520592f));
    REQUIRE(odolVerts.points[0].z == Catch::Approx(7.612178f));
}

// NORMAL DATA PARITY

TEST_CASE("Parity: complex_vehicle LOD 0 Normals - MLOD vs ODOL storage", "[mlod][parity][normals]")
{
    auto mlodData = loadFile(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));
    QIStream mlodStream(mlodData.data(), static_cast<int>(mlodData.size()));
    Poseidon::Asset::Formats::BinaryReader mlodReader(mlodStream);

    auto odolData = loadFile(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    QIStream odolStream(odolData.data(), static_cast<int>(odolData.size()));
    Poseidon::Asset::Formats::BinaryReader odolReader(odolStream);

    auto mlodHeader = Poseidon::Asset::Formats::MLOD::readHeader(mlodReader);
    auto sp3x = Poseidon::Asset::Formats::MLOD::readSP3XHeader(mlodReader);
    auto mlodVerts = Poseidon::Asset::Formats::MLOD::readVertexTable(mlodReader, sp3x.nPos);
    auto mlodNorms = Poseidon::Asset::Formats::MLOD::readNormalTable(mlodReader, sp3x.nNorm);

    auto odolHeader = Poseidon::Asset::Formats::P3D::readHeader(odolReader);
    auto odolVerts = Poseidon::Asset::Formats::P3D::readVertexTable(odolReader);

    // MLOD stores shared normals (more than vertices)
    REQUIRE(mlodNorms.normals.size() == 4815);
    REQUIRE(odolVerts.normals.size() == 2617);

    // Check specific MLOD normals
    REQUIRE(mlodNorms.normals[0].x == Catch::Approx(0.0f));
    REQUIRE(mlodNorms.normals[0].y == Catch::Approx(-1.0f));
    REQUIRE(mlodNorms.normals[0].z == Catch::Approx(0.0f));

    REQUIRE(mlodNorms.normals[4814].x == Catch::Approx(1.119743e-05f));
    REQUIRE(mlodNorms.normals[4814].y == Catch::Approx(-0.3746076f));
    REQUIRE(mlodNorms.normals[4814].z == Catch::Approx(-0.9271834f));

    REQUIRE(mlodNorms.normals[2407].x == Catch::Approx(-0.8375758f));
    REQUIRE(mlodNorms.normals[2407].y == Catch::Approx(-0.5438991f));
    REQUIRE(mlodNorms.normals[2407].z == Catch::Approx(0.05138568f));
}

// FACE DATA PARITY

TEST_CASE("Parity: complex_vehicle LOD 0 Faces - MLOD structure", "[mlod][parity][faces]")
{
    auto mlodData = loadFile(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));
    QIStream mlodStream(mlodData.data(), static_cast<int>(mlodData.size()));
    Poseidon::Asset::Formats::BinaryReader mlodReader(mlodStream);

    auto mlodHeader = Poseidon::Asset::Formats::MLOD::readHeader(mlodReader);
    auto sp3x = Poseidon::Asset::Formats::MLOD::readSP3XHeader(mlodReader);
    auto mlodVerts = Poseidon::Asset::Formats::MLOD::readVertexTable(mlodReader, sp3x.nPos);
    auto mlodNorms = Poseidon::Asset::Formats::MLOD::readNormalTable(mlodReader, sp3x.nNorm);
    auto mlodFaces = Poseidon::Asset::Formats::MLOD::readFaceTable(mlodReader, sp3x.nFace);

    // Face count
    REQUIRE(mlodFaces.faces.size() == 1341);

    // Check first face structure
    const auto& firstFace = mlodFaces.faces[0];
    REQUIRE(firstFace.n >= 3);
    REQUIRE(firstFace.n <= 4);

    SECTION("Face vertex counts")
    {
        int triangles = 0;
        int quads = 0;

        for (const auto& face : mlodFaces.faces)
        {
            if (face.n == 3)
                triangles++;
            else if (face.n == 4)
                quads++;
        }

        INFO("Triangles: " << triangles);
        INFO("Quads: " << quads);

        REQUIRE(triangles > 0);
        REQUIRE(quads > 0);
    }

    SECTION("Face indices validity")
    {
        // Sample faces (first, last, middle) instead of all 1341
        const auto& face0 = mlodFaces.faces[0];
        for (int i = 0; i < face0.n; ++i)
        {
            REQUIRE(face0.vs[i].point >= 0);
            REQUIRE(face0.vs[i].point < sp3x.nPos);
            REQUIRE(face0.vs[i].normal >= 0);
            REQUIRE(face0.vs[i].normal < sp3x.nNorm);
            REQUIRE(std::isfinite(face0.vs[i].mapU));
            REQUIRE(std::isfinite(face0.vs[i].mapV));
        }

        const auto& faceLast = mlodFaces.faces.back();
        for (int i = 0; i < faceLast.n; ++i)
        {
            REQUIRE(faceLast.vs[i].point >= 0);
            REQUIRE(faceLast.vs[i].point < sp3x.nPos);
            REQUIRE(faceLast.vs[i].normal >= 0);
            REQUIRE(faceLast.vs[i].normal < sp3x.nNorm);
            REQUIRE(std::isfinite(faceLast.vs[i].mapU));
            REQUIRE(std::isfinite(faceLast.vs[i].mapV));
        }

        const auto& faceMid = mlodFaces.faces[670];
        for (int i = 0; i < faceMid.n; ++i)
        {
            REQUIRE(faceMid.vs[i].point >= 0);
            REQUIRE(faceMid.vs[i].point < sp3x.nPos);
            REQUIRE(faceMid.vs[i].normal >= 0);
            REQUIRE(faceMid.vs[i].normal < sp3x.nNorm);
            REQUIRE(std::isfinite(faceMid.vs[i].mapU));
            REQUIRE(std::isfinite(faceMid.vs[i].mapV));
        }
    }

    SECTION("Face textures")
    {
        int emptyTextures = 0;
        int texturedFaces = 0;

        for (const auto& face : mlodFaces.faces)
        {
            if (face.texture[0] == '\0')
            {
                emptyTextures++;
            }
            else
            {
                texturedFaces++;
            }
        }

        INFO("Empty textures: " << emptyTextures);
        INFO("Textured faces: " << texturedFaces);
        REQUIRE(texturedFaces > 0);
    }
}

// NAMED SELECTIONS PARITY

TEST_CASE("Parity: complex_vehicle Named Selections - MLOD TAGG vs ODOL", "[mlod][parity][selections]")
{
    auto mlodData = loadFile(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));
    QIStream mlodStream(mlodData.data(), static_cast<int>(mlodData.size()));
    Poseidon::Asset::Formats::BinaryReader mlodReader(mlodStream);

    auto odolData = loadFile(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    QIStream odolStream(odolData.data(), static_cast<int>(odolData.size()));
    Poseidon::Asset::Formats::BinaryReader odolReader(odolStream);

    // Read MLOD
    auto mlodHeader = Poseidon::Asset::Formats::MLOD::readHeader(mlodReader);
    auto sp3x = Poseidon::Asset::Formats::MLOD::readSP3XHeader(mlodReader);
    auto mlodVerts = Poseidon::Asset::Formats::MLOD::readVertexTable(mlodReader, sp3x.nPos);
    auto mlodNorms = Poseidon::Asset::Formats::MLOD::readNormalTable(mlodReader, sp3x.nNorm);
    auto mlodFaces = Poseidon::Asset::Formats::MLOD::readFaceTable(mlodReader, sp3x.nFace);
    auto mlodTagg = Poseidon::Asset::Formats::MLOD::readTAGGSection(mlodReader, sp3x.nPos, sp3x.nFace);

    // Read ODOL (up to named selections)
    auto odolHeader = Poseidon::Asset::Formats::P3D::readHeader(odolReader);
    auto odolVerts = Poseidon::Asset::Formats::P3D::readVertexTable(odolReader);
    auto odolBounds = Poseidon::Asset::Formats::P3D::readLODBounds(odolReader);
    auto odolTextures = Poseidon::Asset::Formats::P3D::readTextures(odolReader);
    auto odolEdges = Poseidon::Asset::Formats::P3D::readLODEdges(odolReader);
    auto odolFaces = Poseidon::Asset::Formats::P3D::readFaces(odolReader);
    auto odolSections = Poseidon::Asset::Formats::P3D::readSections(odolReader);
    auto odolSelections = Poseidon::Asset::Formats::P3D::readNamedSelections(odolReader);

    // Same count of named selections
    REQUIRE(mlodTagg.namedSelections.size() == 30);
    REQUIRE(odolSelections.selections.size() == 30);
    REQUIRE(mlodTagg.namedSelections.size() == odolSelections.selections.size());

    // Selection names match
    REQUIRE(mlodTagg.namedSelections[0].name == "velka vrtule");
    REQUIRE(odolSelections.selections[0].name == "velka vrtule");

    REQUIRE(mlodTagg.namedSelections[1].name == "mala vrtule");
    REQUIRE(odolSelections.selections[1].name == "mala vrtule");

    REQUIRE(mlodTagg.namedSelections[10].name == "proxy:cargo.01");
    REQUIRE(odolSelections.selections[10].name == "proxy:cargo.01");

    REQUIRE(mlodTagg.namedSelections.back().name == "proxy:flag_alone.01");
    REQUIRE(odolSelections.selections.back().name == "proxy:flag_alone.01");

    // Verify all selection names match
    for (size_t i = 0; i < mlodTagg.namedSelections.size(); ++i)
    {
        REQUIRE(mlodTagg.namedSelections[i].name == odolSelections.selections[i].name);
    }
}

// NAMED PROPERTIES PARITY

TEST_CASE("Parity: complex_vehicle Named Properties - MLOD TAGG vs ODOL", "[mlod][parity][properties]")
{
    auto mlodData = loadFile(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));
    QIStream mlodStream(mlodData.data(), static_cast<int>(mlodData.size()));
    Poseidon::Asset::Formats::BinaryReader mlodReader(mlodStream);

    auto odolData = loadFile(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    QIStream odolStream(odolData.data(), static_cast<int>(odolData.size()));
    Poseidon::Asset::Formats::BinaryReader odolReader(odolStream);

    // Read MLOD
    auto mlodHeader = Poseidon::Asset::Formats::MLOD::readHeader(mlodReader);
    auto sp3x = Poseidon::Asset::Formats::MLOD::readSP3XHeader(mlodReader);
    auto mlodVerts = Poseidon::Asset::Formats::MLOD::readVertexTable(mlodReader, sp3x.nPos);
    auto mlodNorms = Poseidon::Asset::Formats::MLOD::readNormalTable(mlodReader, sp3x.nNorm);
    auto mlodFaces = Poseidon::Asset::Formats::MLOD::readFaceTable(mlodReader, sp3x.nFace);
    auto mlodTagg = Poseidon::Asset::Formats::MLOD::readTAGGSection(mlodReader, sp3x.nPos, sp3x.nFace);

    // Read ODOL (up to named properties)
    auto odolHeader = Poseidon::Asset::Formats::P3D::readHeader(odolReader);
    auto odolVerts = Poseidon::Asset::Formats::P3D::readVertexTable(odolReader);
    auto odolBounds = Poseidon::Asset::Formats::P3D::readLODBounds(odolReader);
    auto odolTextures = Poseidon::Asset::Formats::P3D::readTextures(odolReader);
    auto odolEdges = Poseidon::Asset::Formats::P3D::readLODEdges(odolReader);
    auto odolFaces = Poseidon::Asset::Formats::P3D::readFaces(odolReader);
    auto odolSections = Poseidon::Asset::Formats::P3D::readSections(odolReader);
    auto odolSelections = Poseidon::Asset::Formats::P3D::readNamedSelections(odolReader);
    auto odolProperties = Poseidon::Asset::Formats::P3D::readNamedProperties(odolReader);

    // Same count of named properties
    REQUIRE(mlodTagg.namedProperties.size() == 1);
    REQUIRE(odolProperties.properties.size() == 1);
    REQUIRE(mlodTagg.namedProperties.size() == odolProperties.properties.size());

    // Property matches
    REQUIRE(mlodTagg.namedProperties[0].property == "lodnoshadow");
    REQUIRE(mlodTagg.namedProperties[0].value == "1");
    REQUIRE(odolProperties.properties[0].property == "lodnoshadow");
    REQUIRE(odolProperties.properties[0].value == "1");

    // Verify property data matches exactly
    REQUIRE(mlodTagg.namedProperties[0].property == odolProperties.properties[0].property);
    REQUIRE(mlodTagg.namedProperties[0].value == odolProperties.properties[0].value);
}

// RESOLUTION PARITY

TEST_CASE("Parity: complex_vehicle Resolution - MLOD TAGG", "[mlod][parity][resolution]")
{
    auto mlodData = loadFile(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));
    QIStream mlodStream(mlodData.data(), static_cast<int>(mlodData.size()));
    Poseidon::Asset::Formats::BinaryReader mlodReader(mlodStream);

    auto mlodHeader = Poseidon::Asset::Formats::MLOD::readHeader(mlodReader);
    auto sp3x = Poseidon::Asset::Formats::MLOD::readSP3XHeader(mlodReader);
    auto mlodVerts = Poseidon::Asset::Formats::MLOD::readVertexTable(mlodReader, sp3x.nPos);
    auto mlodNorms = Poseidon::Asset::Formats::MLOD::readNormalTable(mlodReader, sp3x.nNorm);
    auto mlodFaces = Poseidon::Asset::Formats::MLOD::readFaceTable(mlodReader, sp3x.nFace);
    auto mlodTagg = Poseidon::Asset::Formats::MLOD::readTAGGSection(mlodReader, sp3x.nPos, sp3x.nFace);

    // Resolution for LOD 0 should be 0.75
    REQUIRE(mlodTagg.resolution == Catch::Approx(0.75f));
}
