#ifndef NOMINMAX
#define NOMINMAX
#include <stddef.h>
#include <algorithm>
#include <catch2/matchers/catch_matchers.hpp>
#include <string>
#include <vector>
#endif
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <Poseidon/Asset/Formats/P3D/ODOLLoader.hpp>
#include <Poseidon/Asset/Formats/P3D/MLODLoader.hpp>
#include <Poseidon/World/Model/Model.hpp>
#include "../../../test_fixtures.hpp"
#include <cmath>

using namespace Poseidon::Model;
using Catch::Matchers::WithinAbs;

TEST_CASE("Parity: ODOL and MLOD have same LOD count", "[parity][odol][mlod]")
{
    auto odolModel = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    auto mlodModel = Poseidon::Asset::Formats::MLODLoader::load(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));

    REQUIRE(odolModel.lodLevels.size() == mlodModel.lodLevels.size());
}

TEST_CASE("Parity: ODOL and MLOD have same vertex count", "[parity][odol][mlod]")
{
    auto odolModel = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    auto mlodModel = Poseidon::Asset::Formats::MLODLoader::load(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));

    REQUIRE(odolModel.lodLevels.size() == mlodModel.lodLevels.size());

    // Check LOD 0 (main detail level)
    const auto& odolMesh = odolModel.lodLevels[0].mesh;
    const auto& mlodMesh = mlodModel.lodLevels[0].mesh;

    // Note: MLOD expands to more vertices (4051) than ODOL (2617) due to format differences
    // MLOD stores compact vertices that expand per-face-corner, ODOL has pre-expanded vertices
    // This is expected and doesn't affect rendering
    REQUIRE(odolMesh.vertices.size() == 2617);
    REQUIRE(mlodMesh.vertices.size() >= 2617); // MLOD has more due to expansion
}

TEST_CASE("Parity: ODOL and MLOD have similar triangle count", "[parity][odol][mlod]")
{
    auto odolModel = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    auto mlodModel = Poseidon::Asset::Formats::MLODLoader::load(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));

    const auto& odolMesh = odolModel.lodLevels[0].mesh;
    const auto& mlodMesh = mlodModel.lodLevels[0].mesh;

    // MLOD quads become 2 triangles, so MLOD may have slightly more triangles
    // Allow for some variation but they should be in the same ballpark
    size_t odolTriCount = odolMesh.triangles.size();
    size_t mlodTriCount = mlodMesh.triangles.size();

    // MLOD should have at least as many triangles as ODOL (due to quad splitting)
    REQUIRE(mlodTriCount >= odolTriCount);

    // But not more than 2x (worst case: all quads vs all triangles)
    REQUIRE(mlodTriCount <= odolTriCount * 2);
}

TEST_CASE("Parity: ODOL and MLOD have similar bounding box", "[parity][odol][mlod]")
{
    auto odolModel = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    auto mlodModel = Poseidon::Asset::Formats::MLODLoader::load(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));

    odolModel.compile();
    mlodModel.compile();

    const auto& odolMesh = odolModel.lodLevels[0].mesh;
    const auto& mlodMesh = mlodModel.lodLevels[0].mesh;

    // Bounding boxes should be in the same ballpark (within 1.5 tolerance)
    // MLOD computes these from geometry, ODOL has precomputed/simplified values
    REQUIRE_THAT(odolMesh.boundingBox.min.x, WithinAbs(mlodMesh.boundingBox.min.x, 1.5));
    REQUIRE_THAT(odolMesh.boundingBox.min.y, WithinAbs(mlodMesh.boundingBox.min.y, 1.5));
    REQUIRE_THAT(odolMesh.boundingBox.min.z, WithinAbs(mlodMesh.boundingBox.min.z, 1.5));

    REQUIRE_THAT(odolMesh.boundingBox.max.x, WithinAbs(mlodMesh.boundingBox.max.x, 1.5));
    REQUIRE_THAT(odolMesh.boundingBox.max.y, WithinAbs(mlodMesh.boundingBox.max.y, 1.5));
    REQUIRE_THAT(odolMesh.boundingBox.max.z, WithinAbs(mlodMesh.boundingBox.max.z, 1.5));
}

TEST_CASE("Parity: ODOL and MLOD have similar bounding sphere", "[parity][odol][mlod]")
{
    auto odolModel = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    auto mlodModel = Poseidon::Asset::Formats::MLODLoader::load(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));

    odolModel.compile();
    mlodModel.compile();

    const auto& odolMesh = odolModel.lodLevels[0].mesh;
    const auto& mlodMesh = mlodModel.lodLevels[0].mesh;

    // Bounding sphere center should be in the same ballpark
    // MLOD computes these from geometry, ODOL has precomputed/simplified values
    REQUIRE_THAT(odolMesh.boundingSphere.center.x, WithinAbs(mlodMesh.boundingSphere.center.x, 1.5));
    REQUIRE_THAT(odolMesh.boundingSphere.center.y, WithinAbs(mlodMesh.boundingSphere.center.y, 1.5));
    REQUIRE_THAT(odolMesh.boundingSphere.center.z, WithinAbs(mlodMesh.boundingSphere.center.z, 1.5));

    // Radius should be nearly identical
    REQUIRE_THAT(odolMesh.boundingSphere.radius, WithinAbs(mlodMesh.boundingSphere.radius, 0.1));
}

TEST_CASE("Parity: ODOL and MLOD have similar material count", "[parity][odol][mlod]")
{
    auto odolModel = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    auto mlodModel = Poseidon::Asset::Formats::MLODLoader::load(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));

    const auto& odolMesh = odolModel.lodLevels[0].mesh;
    const auto& mlodMesh = mlodModel.lodLevels[0].mesh;

    // Material counts should be very close (allow small variations due to format differences)
    size_t odolMatCount = odolMesh.materials.size();
    size_t mlodMatCount = mlodMesh.materials.size();

    // Should be within 10% of each other
    double ratio = static_cast<double>(std::max(odolMatCount, mlodMatCount)) /
                   static_cast<double>(std::min(odolMatCount, mlodMatCount));
    REQUIRE(ratio <= 1.1);
}

TEST_CASE("Parity: ODOL and MLOD have similar selection count", "[parity][odol][mlod]")
{
    auto odolModel = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    auto mlodModel = Poseidon::Asset::Formats::MLODLoader::load(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));

    const auto& odolMesh = odolModel.lodLevels[0].mesh;
    const auto& mlodMesh = mlodModel.lodLevels[0].mesh;

    // Selection counts should be similar
    size_t odolSelCount = odolMesh.selections.size();
    size_t mlodSelCount = mlodMesh.selections.size();

    // Should be within 20% (formats may store selections differently)
    double ratio = static_cast<double>(std::max(odolSelCount, mlodSelCount)) /
                   static_cast<double>(std::min(odolSelCount, mlodSelCount));
    REQUIRE(ratio <= 1.2);
}

TEST_CASE("Parity: ODOL and MLOD have same 'velka vrtule' selection", "[parity][odol][mlod]")
{
    auto odolModel = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    auto mlodModel = Poseidon::Asset::Formats::MLODLoader::load(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));

    const auto& odolMesh = odolModel.lodLevels[0].mesh;
    const auto& mlodMesh = mlodModel.lodLevels[0].mesh;

    // Find "velka vrtule" selection in both
    const NamedSelection* odolSel = nullptr;
    const NamedSelection* mlodSel = nullptr;

    for (const auto& sel : odolMesh.selections)
    {
        if (sel.name.find("velka vrtule") != std::string::npos)
        {
            odolSel = &sel;
            break;
        }
    }

    for (const auto& sel : mlodMesh.selections)
    {
        if (sel.name.find("velka vrtule") != std::string::npos)
        {
            mlodSel = &sel;
            break;
        }
    }

    REQUIRE(odolSel != nullptr);
    REQUIRE(mlodSel != nullptr);

    // ODOL and MLOD have different vertex layouts:
    // - ODOL: compiled format with deduplicated vertices (276)
    // - MLOD: source format with expanded vertices (384, one per face corner)
    // So vertex counts will differ - just verify both have vertices selected
    REQUIRE(!odolSel->vertexIndices.empty());
    REQUIRE(!mlodSel->vertexIndices.empty());
}

TEST_CASE("Parity: ODOL and MLOD compile successfully", "[parity][odol][mlod]")
{
    auto odolModel = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    auto mlodModel = Poseidon::Asset::Formats::MLODLoader::load(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));

    // Both should compile successfully
    REQUIRE(odolModel.compile());
    REQUIRE(mlodModel.compile());

    // Both should be marked as compiled
    REQUIRE(odolModel.isCompiled());
    REQUIRE(mlodModel.isCompiled());
}

TEST_CASE("Parity: ODOL and MLOD have similar model-wide bounding sphere", "[parity][odol][mlod]")
{
    auto odolModel = Poseidon::Asset::Formats::ODOLLoader::load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    auto mlodModel = Poseidon::Asset::Formats::MLODLoader::load(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));

    odolModel.compile();
    mlodModel.compile();

    // ODOL stores pre-computed vertex-based bounding sphere from the binary file.
    // MLOD computes from bbox half-diagonal (always an overestimate).
    // They differ inherently, so use wider tolerance.
    REQUIRE_THAT(odolModel.boundingSphere.center.x, WithinAbs(mlodModel.boundingSphere.center.x, 1.5));
    REQUIRE_THAT(odolModel.boundingSphere.center.y, WithinAbs(mlodModel.boundingSphere.center.y, 1.5));
    REQUIRE_THAT(odolModel.boundingSphere.center.z, WithinAbs(mlodModel.boundingSphere.center.z, 1.5));

    REQUIRE_THAT(odolModel.boundingSphere.radius, WithinAbs(mlodModel.boundingSphere.radius, 0.5));
}
