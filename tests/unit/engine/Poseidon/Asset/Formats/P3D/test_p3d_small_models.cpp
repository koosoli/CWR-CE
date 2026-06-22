// test_p3d_small_models.cpp - Tests for small simple models (light_disc.p3d, flat_quad.p3d)
// Validates the loader handles trivially small models without crash

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/Asset/Formats/P3D/ODOLLoader.hpp>
#include <Poseidon/World/Model/Model.hpp>
#include "../../../test_fixtures.hpp"
#include <string>
#include <vector>

using namespace Poseidon::Model;

TEST_CASE("ODOLLoader: light_disc.p3d - small model", "[odol][loader][small]")
{
    auto model = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/light_disc.p3d"));
    REQUIRE(model.compile());

    REQUIRE(model.sourceFormat == "ODOL");
    REQUIRE(model.sourceVersion == 7);
    REQUIRE(model.lodLevels.size() == 1);
    REQUIRE(model.isCompiled());

    const auto& mesh = model.lodLevels[0].mesh;

    // Geometry: single quad, 4 vertices, no triangles
    REQUIRE(mesh.vertices.size() == 4);
    REQUIRE(mesh.triangles.size() == 0);
    REQUIRE(mesh.quads.size() == 1);

    // Metadata
    REQUIRE(mesh.materials.size() == 1);
    REQUIRE(mesh.sections.size() == 1);
    REQUIRE(mesh.selections.size() == 0);
    REQUIRE(mesh.properties.size() == 0);
    REQUIRE(mesh.proxies.size() == 0);

    // Bounding sphere: large radius (light_disc billboard)
    REQUIRE(mesh.boundingSphere.center.x == Catch::Approx(0.0f));
    REQUIRE(mesh.boundingSphere.center.y == Catch::Approx(0.0f));
    REQUIRE(mesh.boundingSphere.center.z == Catch::Approx(0.0f));
    REQUIRE(mesh.boundingSphere.radius == Catch::Approx(887.349731f));

    // Bounding box: flat plane in Z
    REQUIRE(mesh.boundingBox.min.x == Catch::Approx(-627.450989f));
    REQUIRE(mesh.boundingBox.min.y == Catch::Approx(-627.450989f));
    REQUIRE(mesh.boundingBox.min.z == Catch::Approx(0.0f));
    REQUIRE(mesh.boundingBox.max.x == Catch::Approx(627.450989f));
    REQUIRE(mesh.boundingBox.max.y == Catch::Approx(627.450989f));
    REQUIRE(mesh.boundingBox.max.z == Catch::Approx(0.0f));

    // Sample vertices
    REQUIRE(mesh.vertices[0].position.x == Catch::Approx(-627.450989f));
    REQUIRE(mesh.vertices.back().position.x == Catch::Approx(627.450989f));

    REQUIRE(mesh.iconColor == 4278190080u);
    REQUIRE(mesh.selectedColor == 0u);
}

TEST_CASE("ODOLLoader: flat_quad.p3d - small model", "[odol][loader][small]")
{
    auto model = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/flat_quad.p3d"));
    REQUIRE(model.compile());

    REQUIRE(model.sourceFormat == "ODOL");
    REQUIRE(model.sourceVersion == 7);
    REQUIRE(model.lodLevels.size() == 1);
    REQUIRE(model.isCompiled());

    const auto& mesh = model.lodLevels[0].mesh;

    // Geometry: single quad, 4 vertices, no triangles
    REQUIRE(mesh.vertices.size() == 4);
    REQUIRE(mesh.triangles.size() == 0);
    REQUIRE(mesh.quads.size() == 1);

    // Metadata
    REQUIRE(mesh.materials.size() == 1);
    REQUIRE(mesh.sections.size() == 1);
    REQUIRE(mesh.selections.size() == 0);
    REQUIRE(mesh.properties.size() == 0);
    REQUIRE(mesh.proxies.size() == 0);

    // Bounding sphere: small flat_quadangle
    REQUIRE(mesh.boundingSphere.center.x == Catch::Approx(0.0f));
    REQUIRE(mesh.boundingSphere.center.y == Catch::Approx(0.0f));
    REQUIRE(mesh.boundingSphere.center.z == Catch::Approx(0.0f));
    REQUIRE(mesh.boundingSphere.radius == Catch::Approx(2.209709f));

    // Bounding box: flat plane in Y
    REQUIRE(mesh.boundingBox.min.x == Catch::Approx(-1.562500f));
    REQUIRE(mesh.boundingBox.min.y == Catch::Approx(0.0f));
    REQUIRE(mesh.boundingBox.min.z == Catch::Approx(-1.562500f));
    REQUIRE(mesh.boundingBox.max.x == Catch::Approx(1.562500f));
    REQUIRE(mesh.boundingBox.max.y == Catch::Approx(0.0f));
    REQUIRE(mesh.boundingBox.max.z == Catch::Approx(1.562500f));

    // Sample vertices
    REQUIRE(mesh.vertices[0].position.x == Catch::Approx(-1.562500f));
    REQUIRE(mesh.vertices[0].position.z == Catch::Approx(-1.562500f));
    REQUIRE(mesh.vertices.back().position.x == Catch::Approx(-1.562500f));
    REQUIRE(mesh.vertices.back().position.z == Catch::Approx(1.562500f));

    REQUIRE(mesh.iconColor == 0u);
    REQUIRE(mesh.selectedColor == 0u);
}
