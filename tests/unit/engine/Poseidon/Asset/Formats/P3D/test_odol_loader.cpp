// test_odol_loader.cpp - Tests for ODOL -> Model loader
// Validates that ODOLLoader correctly converts ODOL binary format to unified Model

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <vector>
#include <Poseidon/Asset/Formats/P3D/ODOLLoader.hpp>
#include <Poseidon/World/Model/Model.hpp>
#include "../../../test_fixtures.hpp"
#include <stdint.h>
#include <string>
#include <vector>

using namespace Poseidon::Model;

// fuzz_p3d regression: a wire array count must be rejected before it drives a huge
// allocation. "ODOL" v7 / 1 LOD, then a vertex-table clipFlags compressed-array
// count of 0x08000000 (134M * 4B = 536MB > the 256MB cap). Pre-fix
// readCompressedArray did std::vector(count) up front (OOM); the cap now throws.
TEST_CASE("ODOLLoader: malformed array count is rejected before allocation", "[odol][loader][fuzz]")
{
    std::vector<char> buf;
    auto put32 = [&](uint32_t v)
    {
        for (int i = 0; i < 4; ++i)
            buf.push_back(static_cast<char>((v >> (8 * i)) & 0xFF));
    };
    buf.push_back('O');
    buf.push_back('D');
    buf.push_back('O');
    buf.push_back('L');
    put32(7);          // version
    put32(1);          // lodCount
    put32(0x08000000); // clipFlags compressed-array element count

    REQUIRE_THROWS_WITH(
        Poseidon::Asset::Formats::ODOLLoader::loadFromBuffer(buf.data(), static_cast<int>(buf.size()), "fuzz"),
        Catch::Matchers::ContainsSubstring("count"));
}

TEST_CASE("ODOLLoader: Basic loading - complex_vehicle.p3d", "[odol][loader]")
{
    auto model = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));

    // Compile to compute missing data
    REQUIRE(model.compile());

    // Basic model metadata
    REQUIRE(model.sourceFormat == "ODOL");
    REQUIRE(model.sourceVersion == 7);
    REQUIRE(model.sourcePath == GET_FIXTURE("p3d/complex_vehicle.p3d"));

    // LOD count
    REQUIRE(model.lodLevels.size() == 19);

    // Model should be compiled
    REQUIRE(model.isCompiled());
}

TEST_CASE("ODOLLoader: Geometry - LOD 0", "[odol][loader][geometry]")
{
    auto model = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    model.compile();

    REQUIRE(model.lodLevels.size() > 0);
    const auto& lod0 = model.lodLevels[0];
    const auto& mesh = lod0.mesh;

    // Vertex count
    REQUIRE(mesh.vertices.size() == 2617);

    // Sample vertex positions
    REQUIRE(mesh.vertices[0].position.x == Catch::Approx(0.205741f));
    REQUIRE(mesh.vertices[0].position.y == Catch::Approx(2.520592f));
    REQUIRE(mesh.vertices[0].position.z == Catch::Approx(7.612178f));

    REQUIRE(mesh.vertices.back().position.x == Catch::Approx(0.085317f));
    REQUIRE(mesh.vertices.back().position.y == Catch::Approx(-1.780597f));
    REQUIRE(mesh.vertices.back().position.z == Catch::Approx(-1.890425f));

    // Sample UVs
    REQUIRE(mesh.vertices[0].uv.u == Catch::Approx(0.005356f));
    REQUIRE(mesh.vertices[0].uv.v == Catch::Approx(-0.001051f));

    REQUIRE(mesh.vertices.back().uv.u == Catch::Approx(0.490648f));
    REQUIRE(mesh.vertices.back().uv.v == Catch::Approx(1.332385f));

    // Face count (1341 faces: 549 triangles + 792 quads)
    REQUIRE(mesh.triangles.size() == 549);
    REQUIRE(mesh.quads.size() == 792);
    REQUIRE(mesh.triangles.size() + mesh.quads.size() == 1341);
}

TEST_CASE("ODOLLoader: Materials - LOD 0", "[odol][loader][materials]")
{
    auto model = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    model.compile();

    const auto& mesh = model.lodLevels[0].mesh;

    // Material count (from textures)
    REQUIRE(mesh.materials.size() == 69);

    // Sample materials
    REQUIRE(mesh.materials[0].name == "data\\uh_rotorosa_side.pac");
    REQUIRE(mesh.materials[0].texturePath == "data\\uh_rotorosa_side.pac");

    REQUIRE(mesh.materials[10].name == "data\\complex_vehicle_list_vrtule.pac");
    REQUIRE(mesh.materials.back().name == "");
}

TEST_CASE("ODOLLoader: Sections - LOD 0", "[odol][loader][sections]")
{
    auto model = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    model.compile();

    const auto& mesh = model.lodLevels[0].mesh;

    // Section count
    REQUIRE(mesh.sections.size() == 101);

    // Sample sections
    REQUIRE(mesh.sections[0].materialIndex == UINT32_MAX);
    REQUIRE(mesh.sections[0].startTriangle == 0);
    REQUIRE(mesh.sections[0].triangleCount == 240);
}

TEST_CASE("ODOLLoader: Named Selections - LOD 0", "[odol][loader][selections]")
{
    auto model = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    model.compile();

    const auto& mesh = model.lodLevels[0].mesh;

    // Selection count
    REQUIRE(mesh.selections.size() == 30);

    // Sample selection names
    REQUIRE(mesh.selections[0].name == "velka vrtule"); // "big propeller"
    REQUIRE(mesh.selections[1].name == "mala vrtule");  // "small propeller"
    REQUIRE(mesh.selections[10].name == "proxy:cargo.01");

    // Sample selection has vertices
    REQUIRE(mesh.selections[0].vertexIndices.size() == 276);
}

TEST_CASE("ODOLLoader: Named Properties - LOD 0", "[odol][loader][properties]")
{
    auto model = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    model.compile();

    const auto& mesh = model.lodLevels[0].mesh;

    // Property count
    REQUIRE(mesh.properties.size() == 1);

    // Sample property
    REQUIRE(mesh.properties[0].name == "lodnoshadow");
    REQUIRE(mesh.properties[0].value == "1");
}

TEST_CASE("ODOLLoader: Proxies - LOD 0", "[odol][loader][proxies]")
{
    auto model = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    model.compile();

    const auto& mesh = model.lodLevels[0].mesh;

    // Proxy count
    REQUIRE(mesh.proxies.size() == 15);

    // Sample proxy names (short names from ODOL)
    REQUIRE(mesh.proxies[0].name == "complex_vehiclepilot");
    REQUIRE(mesh.proxies[1].name == "cargo");
    REQUIRE(mesh.proxies.back().name == "flag_alone");
}

TEST_CASE("ODOLLoader: Edges - LOD 0", "[odol][loader][edges]")
{
    auto model = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    model.compile();

    const auto& mesh = model.lodLevels[0].mesh;

    // Edge counts
    REQUIRE(mesh.edges.mlodIndices.size() == 1679);
    REQUIRE(mesh.edges.vertexIndices.size() == 2617);

    // Sample edges
    REQUIRE(mesh.edges.mlodIndices[0] == 1);
    REQUIRE(mesh.edges.mlodIndices[100] == 155);

    REQUIRE(mesh.edges.vertexIndices[0] == 0);
    REQUIRE(mesh.edges.vertexIndices[100] == 74);
}

TEST_CASE("ODOLLoader: Bounding volumes - LOD 0", "[odol][loader][bounds]")
{
    auto model = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    model.compile();

    const auto& mesh = model.lodLevels[0].mesh;

    // Bounding box
    REQUIRE(mesh.boundingBox.min.x == Catch::Approx(-8.955186f));
    REQUIRE(mesh.boundingBox.min.y == Catch::Approx(-2.520592f));
    REQUIRE(mesh.boundingBox.min.z == Catch::Approx(-10.471918f));

    REQUIRE(mesh.boundingBox.max.x == Catch::Approx(8.955186f));
    REQUIRE(mesh.boundingBox.max.y == Catch::Approx(2.520592f));
    REQUIRE(mesh.boundingBox.max.z == Catch::Approx(10.447238f));

    // Bounding sphere (computed from box)
    REQUIRE(mesh.boundingSphere.center.x == Catch::Approx(0.0f));
    REQUIRE(mesh.boundingSphere.center.y == Catch::Approx(0.0f));
    REQUIRE(mesh.boundingSphere.center.z == Catch::Approx(-0.01234f).margin(0.1f));
    REQUIRE(mesh.boundingSphere.radius > 0.0f);
}

TEST_CASE("ODOLLoader: LOD end data - LOD 0", "[odol][loader][enddata]")
{
    auto model = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    model.compile();

    const auto& mesh = model.lodLevels[0].mesh;

    // Icon colors (from LOD end data - actual values from file)
    REQUIRE(mesh.iconColor == 1313557311);
    REQUIRE(mesh.selectedColor == 1515410247);
}
