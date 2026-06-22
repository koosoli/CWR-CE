#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Asset/Formats/P3D/P3DStructures.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include "../../../test_fixtures.hpp"
#include <fstream>
#include <vector>
#include <stdint.h>
#include <stdexcept>
#include <string>

using namespace Poseidon::Asset::Formats;

static std::vector<char> loadFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + path);
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    return buffer;
}

TEST_CASE("proxy_structure.p3d: Parse all LODs", "[p3d][proxy_structure]")
{
    auto data = loadFile(GET_FIXTURE("p3d/proxy_structure.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    BinaryReader reader(stream);

    auto header = P3D::readHeader(reader);
    REQUIRE(header.lodCount == 11);

    // Parse all LODs
    std::vector<P3D::CompleteLOD> lods;
    for (uint32_t i = 0; i < header.lodCount; ++i)
    {
        lods.push_back(P3D::readCompleteLOD(reader));
    }

    // Validate LOD 0 - highest detail with proxies
    {
        const auto& lod = lods[0];
        REQUIRE(lod.vertexTable.points.size() == 6037);
        REQUIRE(lod.textures.texturePaths.size() == 33);
        REQUIRE(lod.textures.texturePaths[0] == "data\\dum2_s.pac");
        REQUIRE(lod.faces.count == 2475);
        REQUIRE(lod.sections.sections.size() == 46);
        REQUIRE(lod.namedSelections.selections.size() == 27);
        REQUIRE(lod.namedSelections.selections[0].name == "proxy:hangar_zidle.04");
        REQUIRE(lod.namedSelections.selections[26].name == "!!!stit");
        REQUIRE(lod.namedProperties.properties.size() == 1);
        REQUIRE(lod.frames.frames.size() == 0);
        REQUIRE(lod.proxies.proxies.size() == 26);
    }

    // Validate LOD 5 - medium detail
    {
        const auto& lod = lods[5];
        REQUIRE(lod.vertexTable.points.size() == 127);
        REQUIRE(lod.textures.texturePaths.size() == 8);
        REQUIRE(lod.textures.texturePaths[0] == "data\\tasky_p90.pac");
        REQUIRE(lod.faces.count == 38);
        REQUIRE(lod.sections.sections.size() == 13);
        REQUIRE(lod.namedSelections.selections.size() == 0);
        REQUIRE(lod.namedProperties.properties.size() == 0);
        REQUIRE(lod.frames.frames.size() == 0);
        REQUIRE(lod.proxies.proxies.size() == 0);
    }

    // Validate LOD 10 - roadway LOD with many components
    {
        const auto& lod = lods[10];
        REQUIRE(lod.vertexTable.points.size() == 735);
        REQUIRE(lod.textures.texturePaths.size() == 1);
        REQUIRE(lod.textures.texturePaths[0] == "");
        REQUIRE(lod.faces.count == 577);
        REQUIRE(lod.sections.sections.size() == 1);
        REQUIRE(lod.namedSelections.selections.size() == 89);
        REQUIRE(lod.namedSelections.selections[0].name == "component01");
        REQUIRE(lod.namedSelections.selections[88].name == "component89");
        REQUIRE(lod.namedProperties.properties.size() == 0);
        REQUIRE(lod.frames.frames.size() == 0);
        REQUIRE(lod.proxies.proxies.size() == 0);
    }
}
