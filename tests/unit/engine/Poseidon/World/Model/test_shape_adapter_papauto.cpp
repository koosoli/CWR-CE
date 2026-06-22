#include <catch2/catch_test_macros.hpp>

#include <Poseidon/World/Model/ModelCache.hpp>
#include <Poseidon/World/Model/Model.hpp>
#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>
#include <Poseidon/World/Model/ShapeAdapter.hpp>

#include "../../test_fixtures.hpp"
#include <catch2/catch_message.hpp>
#include <memory>
#include <string>
#include <vector>

using namespace Poseidon::Model;

TEST_CASE("ShapeAdapter: complex_vehicle_mlod.p3d - MLOD format", "[ShapeAdapter][mlod][complex_vehicle]")
{
    const char* modelPath = GET_FIXTURE("mlod/complex_vehicle_mlod.p3d");

    SECTION("Load and convert")
    {
        Poseidon::ModelCache cache;
        auto model = cache.load(modelPath);
        REQUIRE(model);
        REQUIRE(model->lodLevels.size() > 0);

        LODShapeWithShadow* shape = nullptr;
        REQUIRE_NOTHROW(shape = ShapeAdapter::convertToLODShape(*model));
        REQUIRE(shape != nullptr);

        INFO("Model has " << model->lodLevels.size() << " LODs");
        REQUIRE(shape->NLevels() > 0);

        for (int i = 0; i < shape->NLevels(); ++i)
        {
            Shape* lod = shape->Level(i);
            REQUIRE(lod != nullptr);
            INFO("LOD " << i << " has " << lod->NFaces() << " faces");
            INFO("LOD " << i << " has " << lod->NVertex() << " vertices");
            INFO("LOD " << i << " has " << lod->NSections() << " sections");

            if (lod->NSections() > 0)
            {
                REQUIRE(lod->BeginFaces() <= lod->EndFaces());
            }
        }

        delete shape;
    }
}
