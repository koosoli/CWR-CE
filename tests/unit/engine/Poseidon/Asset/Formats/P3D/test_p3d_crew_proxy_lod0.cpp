// test_p3d_crew_proxy_lod0.cpp - Test crew_proxy.p3d LOD 0 only
// Simple multi-LOD file - just test first LOD for now

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Asset/Formats/P3D/P3DStructures.hpp>
#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include "../../../test_fixtures.hpp"
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

TEST_CASE("crew_proxy.p3d: Complete Model", "[p3d][crew_proxy][multilod]")
{
    auto data = loadFile(GET_FIXTURE("p3d/crew_proxy.p3d"));
    QIStream stream(data.data(), static_cast<int>(data.size()));
    Poseidon::Asset::Formats::BinaryReader reader(stream);

    // Read complete model with all LODs
    auto model = Poseidon::Asset::Formats::P3D::readModel(reader);

    // Verify header - crew_proxy is actually single-LOD despite filename
    REQUIRE(model.header.lodCount == 1);
    REQUIRE(model.lods.size() == 1);

    // Verify LOD 0 has data
    const auto& lod0 = model.lods[0];
    REQUIRE(lod0.vertexTable.points.size() > 0);
    REQUIRE(lod0.faces.count > 0);
}
