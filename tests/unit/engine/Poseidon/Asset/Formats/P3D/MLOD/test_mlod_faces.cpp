// test_mlod_faces.cpp - Tests for MLOD face reading
// Batch 3: Face Data

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Asset/Formats/P3D/MLODStructures.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include "../../../../test_fixtures.hpp"
#include <fstream>
#include <vector>
#include <stddef.h>
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

TEST_CASE("MLOD Complete: complex_vehicle_mlod.p3d LOD 0", "[mlod][complete][batch3]")
{
    auto data = loadFile(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read MLOD header
    auto header = Poseidon::Asset::Formats::MLOD::readHeader(reader);
    REQUIRE(header.lodCount == 19);

    // Read SP3X section for LOD 0
    auto sp3x = Poseidon::Asset::Formats::MLOD::readSP3XHeader(reader);
    REQUIRE(sp3x.nPos == 2617);
    REQUIRE(sp3x.nNorm == 4815);
    REQUIRE(sp3x.nFace == 1341);

    // Read all geometry data
    auto vtable = Poseidon::Asset::Formats::MLOD::readVertexTable(reader, sp3x.nPos);
    REQUIRE(vtable.points.size() == 2617);

    auto ntable = Poseidon::Asset::Formats::MLOD::readNormalTable(reader, sp3x.nNorm);
    REQUIRE(ntable.normals.size() == 4815);

    // Read faces
    auto ftable = Poseidon::Asset::Formats::MLOD::readFaceTable(reader, sp3x.nFace);
    REQUIRE(ftable.faces.size() == 1341);

    SECTION("Face structure validation")
    {
        // Sample faces (first, last, middle) instead of all 1341
        const auto& face0 = ftable.faces[0];
        REQUIRE(face0.n >= 3);
        REQUIRE(face0.n <= 4);
        for (int i = 0; i < face0.n; ++i)
        {
            REQUIRE(face0.vs[i].point >= 0);
            REQUIRE(face0.vs[i].point < sp3x.nPos);
            REQUIRE(face0.vs[i].normal >= 0);
            REQUIRE(face0.vs[i].normal < sp3x.nNorm);
        }

        const auto& faceLast = ftable.faces.back();
        REQUIRE(faceLast.n >= 3);
        REQUIRE(faceLast.n <= 4);
        for (int i = 0; i < faceLast.n; ++i)
        {
            REQUIRE(faceLast.vs[i].point >= 0);
            REQUIRE(faceLast.vs[i].point < sp3x.nPos);
            REQUIRE(faceLast.vs[i].normal >= 0);
            REQUIRE(faceLast.vs[i].normal < sp3x.nNorm);
        }

        const auto& faceMid = ftable.faces[670];
        REQUIRE(faceMid.n >= 3);
        REQUIRE(faceMid.n <= 4);
        for (int i = 0; i < faceMid.n; ++i)
        {
            REQUIRE(faceMid.vs[i].point >= 0);
            REQUIRE(faceMid.vs[i].point < sp3x.nPos);
            REQUIRE(faceMid.vs[i].normal >= 0);
            REQUIRE(faceMid.vs[i].normal < sp3x.nNorm);
        }
    }

    SECTION("Face texture names")
    {
        int texturedFaces = 0;
        for (const auto& face : ftable.faces)
        {
            if (face.texture[0] != '\0')
            {
                texturedFaces++;
            }
        }
        // At least some faces should have textures
        REQUIRE(texturedFaces > 0);
    }
}
