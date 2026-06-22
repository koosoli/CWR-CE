// test_mlod_tagg.cpp - TAGG section tests for MLOD format
// Tests named selections, properties, and mass data

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Asset/Formats/P3D/MLODStructures.hpp>
#include <Poseidon/Asset/Formats/P3D/MLODLoader.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include "../../../../test_fixtures.hpp"
#include <fstream>
#include <iterator>
#include <vector>
#include <stddef.h>
#include <catch2/catch_message.hpp>
#include <stdexcept>
#include <string>

// fuzz_p3d reproducer. readTAGGProperty read
// 64 bytes into a char[64] with no terminator, then std::string(buf) ran strlen off
// the stack when all 64 wire bytes were non-NUL — a stack-buffer-overflow read. The
// +1 NUL-guard byte now bounds it. ASan reports the over-read pre-fix; the loader
// completing (parse or thrown error) without an OOB is the regression's teeth.
TEST_CASE("MLOD TAGG: fuzz reproducer reads property names without stack OOB", "[mlod][tagg][fuzz]")
{
    std::ifstream f(GET_FIXTURE("mlod/fuzz_tagg_oob.p3d"), std::ios::binary);
    REQUIRE(f.good());
    std::vector<char> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    REQUIRE(!data.empty());
    try
    {
        Poseidon::Asset::Formats::MLODLoader::loadFromBuffer(data.data(), static_cast<int>(data.size()), "fuzz.p3d");
    }
    catch (...)
    {
    }
    SUCCEED("MLOD TAGG reproducer parsed without an out-of-bounds read");
}

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

TEST_CASE("MLOD TAGG: complex_vehicle_mlod.p3d - Read TAGG sections", "[mlod][tagg][batch4]")
{
    auto data = loadFile(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read header and SP3X
    auto header = Poseidon::Asset::Formats::MLOD::readHeader(reader);
    auto sp3x = Poseidon::Asset::Formats::MLOD::readSP3XHeader(reader);
    auto vertices = Poseidon::Asset::Formats::MLOD::readVertexTable(reader, sp3x.nPos);
    auto normals = Poseidon::Asset::Formats::MLOD::readNormalTable(reader, sp3x.nNorm);
    auto faces = Poseidon::Asset::Formats::MLOD::readFaceTable(reader, sp3x.nFace);

    // Read TAGG section
    auto tagg = Poseidon::Asset::Formats::MLOD::readTAGGSection(reader, sp3x.nPos, sp3x.nFace);

    // Verify named selections count (should match ODOL: 30)
    REQUIRE(tagg.namedSelections.size() == 30);

    // Check first named selection
    const auto& sel0 = tagg.namedSelections[0];
    REQUIRE(sel0.name == "velka vrtule");           // "big propeller" in Czech
    REQUIRE(sel0.pointWeights.size() == sp3x.nPos); // 2617 points
    REQUIRE(sel0.faceFlags.size() == sp3x.nFace);   // 1341 faces

    // Check another named selection
    const auto& sel1 = tagg.namedSelections[1];
    REQUIRE(sel1.name == "mala vrtule"); // "small propeller" in Czech

    // Check proxy selection
    bool foundCargoProxy = false;
    for (const auto& sel : tagg.namedSelections)
    {
        if (sel.name == "proxy:cargo.01")
        {
            foundCargoProxy = true;
            break;
        }
    }
    REQUIRE(foundCargoProxy);

    // Check last selection
    const auto& selLast = tagg.namedSelections.back();
    REQUIRE(selLast.name == "proxy:flag_alone.01");

    // Verify properties (should have 1: lodnoshadow)
    REQUIRE(tagg.namedProperties.size() == 1);
    REQUIRE(tagg.namedProperties[0].property == "lodnoshadow");
    REQUIRE(tagg.namedProperties[0].value == "1");

    // Verify resolution (LOD 0 has resolution 0.75)
    REQUIRE(tagg.resolution == Catch::Approx(0.75f));
}

TEST_CASE("MLOD TAGG: camera_path_a.p3d - Simple model TAGG", "[mlod][tagg][batch4]")
{
    auto data = loadFile(GET_FIXTURE("mlod/camera_path_a.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read header and SP3X
    auto header = Poseidon::Asset::Formats::MLOD::readHeader(reader);
    auto sp3x = Poseidon::Asset::Formats::MLOD::readSP3XHeader(reader);
    auto vertices = Poseidon::Asset::Formats::MLOD::readVertexTable(reader, sp3x.nPos);
    auto normals = Poseidon::Asset::Formats::MLOD::readNormalTable(reader, sp3x.nNorm);
    auto faces = Poseidon::Asset::Formats::MLOD::readFaceTable(reader, sp3x.nFace);

    // Read TAGG section
    auto tagg = Poseidon::Asset::Formats::MLOD::readTAGGSection(reader, sp3x.nPos, sp3x.nFace);

    // Simple camera model should have fewer selections
    INFO("Named selections: " << tagg.namedSelections.size());
    INFO("Properties: " << tagg.namedProperties.size());

    // Verify all selection arrays match point/face counts
    for (const auto& sel : tagg.namedSelections)
    {
        REQUIRE(sel.pointWeights.size() == sp3x.nPos);
        REQUIRE(sel.faceFlags.size() == sp3x.nFace);
    }
}
