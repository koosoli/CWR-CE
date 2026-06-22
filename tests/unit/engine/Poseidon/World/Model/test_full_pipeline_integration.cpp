// test_full_pipeline_integration.cpp - Integration test for full P3D loader pipeline
// Tests: FormatDetector -> ODOLLoader/MLODLoader -> compile -> ShapeAdapter -> LODShapeWithShadow
// This runs on both x86 and x64 (no old loader dependency).

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <Poseidon/Asset/Formats/Common/FormatDetector.hpp>
#include <memory>
#include <string>
#include <vector>
using Poseidon::Asset::Formats::FormatInfo;
using Poseidon::Asset::Formats::P3DFormatDetector;
#include <Poseidon/Asset/Formats/P3D/ODOLLoader.hpp>
#include <Poseidon/Asset/Formats/P3D/MLODLoader.hpp>
#include <Poseidon/World/Model/Model.hpp>
#include <Poseidon/World/Model/ShapeAdapter.hpp>
#include <Poseidon/World/Model/ModelCache.hpp>

#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>
#include "../../test_fixtures.hpp"

using namespace Poseidon::Model;
using Poseidon::Asset::Formats::MLODLoader;
using Poseidon::Asset::Formats::ODOLLoader;

TEST_CASE("Full pipeline: ODOL -> Model -> LODShapeWithShadow", "[integration][pipeline]")
{
    const char* path = GET_FIXTURE("p3d/complex_vehicle.p3d");

    auto info = P3DFormatDetector::DetectFormat(path);
    REQUIRE(info.isSupported);
    REQUIRE(info.signature == "ODOL");

    Model model = ODOLLoader::load(path);
    REQUIRE(model.compile());
    REQUIRE(model.isCompiled());
    REQUIRE(!model.lodLevels.empty());

    LODShapeWithShadow* shape = ShapeAdapter::convertToLODShape(model);
    REQUIRE(shape != nullptr);
    REQUIRE(shape->NLevels() > 0);

    // Verify the shape has expected structure
    int nLevels = shape->NLevels();
    REQUIRE(nLevels == 19); // complex_vehicle has 19 LODs (visual + shadow + geometry etc)

    // Check first LOD has geometry
    Shape* lod0 = shape->Level(0);
    REQUIRE(lod0 != nullptr);
    REQUIRE(lod0->NVertex() > 0);
    REQUIRE(lod0->NFaces() > 0);
    REQUIRE(lod0->NSections() > 0);

    delete shape;
}

TEST_CASE("Full pipeline: MLOD -> Model -> LODShapeWithShadow", "[integration][pipeline]")
{
    const char* path = GET_FIXTURE("mlod/complex_vehicle_mlod.p3d");

    auto info = P3DFormatDetector::DetectFormat(path);
    REQUIRE(info.isSupported);
    REQUIRE(info.signature == "MLOD");

    Model model = MLODLoader::load(path);
    REQUIRE(model.compile());
    REQUIRE(model.isCompiled());

    LODShapeWithShadow* shape = ShapeAdapter::convertToLODShape(model);
    REQUIRE(shape != nullptr);
    REQUIRE(shape->NLevels() > 0);

    Shape* lod0 = shape->Level(0);
    REQUIRE(lod0 != nullptr);
    REQUIRE(lod0->NVertex() > 0);
    REQUIRE(lod0->NFaces() > 0);

    delete shape;
}

TEST_CASE("Full pipeline: ModelCache loads and caches ODOL", "[integration][pipeline]")
{
    const char* path = GET_FIXTURE("p3d/complex_vehicle.p3d");

    Poseidon::ModelCache cache;
    auto model1 = cache.load(path);
    REQUIRE(model1 != nullptr);
    REQUIRE(model1->isCompiled());

    // Second load should be a cache hit
    auto model2 = cache.load(path);
    REQUIRE(model2 != nullptr);
    REQUIRE(model1.get() == model2.get()); // Same pointer = cached

    auto stats = cache.getStats();
    REQUIRE(stats.totalLoads == 2);
    REQUIRE(stats.cacheHits == 1);
    REQUIRE(stats.cacheMisses == 1);

    // Convert cached model to shape
    LODShapeWithShadow* shape = ShapeAdapter::convertToLODShape(*model1);
    REQUIRE(shape != nullptr);
    REQUIRE(shape->NLevels() == 19);

    delete shape;
}

TEST_CASE("Full pipeline: FormatDetector rejects invalid files", "[integration][pipeline]")
{
    // Non-existent file
    auto info = P3DFormatDetector::DetectFormat("nonexistent_model.p3d");
    REQUIRE_FALSE(info.isSupported);
}
