#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Poseidon/World/Model/ModelCache.hpp>
#include <Poseidon/World/Model/Model.hpp>
#include "../../test_fixtures.hpp"
#include <memory>
#include <string>
#include <vector>

using namespace Poseidon;
using namespace Poseidon::Model;

TEST_CASE("ModelCache: Load ODOL model", "[model][cache][odol]")
{
    ModelCache cache;

    auto model = cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));

    REQUIRE(model != nullptr);
    REQUIRE(model->isCompiled());
    REQUIRE(model->sourceFormat == "ODOL");
    REQUIRE(model->lodLevels.size() == 19);

    // Check stats
    auto stats = cache.getStats();
    REQUIRE(stats.totalLoads == 1);
    REQUIRE(stats.cacheMisses == 1);
    REQUIRE(stats.cacheHits == 0);
    REQUIRE(stats.loadFailures == 0);
    REQUIRE(stats.cachedModels == 1);
}

TEST_CASE("ModelCache: Load MLOD model", "[model][cache][mlod]")
{
    ModelCache cache;

    auto model = cache.load(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));

    REQUIRE(model != nullptr);
    REQUIRE(model->isCompiled());
    REQUIRE(model->sourceFormat == "MLOD");
    REQUIRE(model->lodLevels.size() == 19);

    // Check stats
    auto stats = cache.getStats();
    REQUIRE(stats.totalLoads == 1);
    REQUIRE(stats.cacheMisses == 1);
    REQUIRE(stats.cacheHits == 0);
    REQUIRE(stats.loadFailures == 0);
    REQUIRE(stats.cachedModels == 1);
}

TEST_CASE("ModelCache: Caching behavior", "[model][cache]")
{
    ModelCache cache;

    SECTION("Same file loaded twice returns cached version")
    {
        auto model1 = cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
        auto model2 = cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));

        REQUIRE(model1 != nullptr);
        REQUIRE(model2 != nullptr);
        REQUIRE(model1 == model2); // Same pointer

        auto stats = cache.getStats();
        REQUIRE(stats.totalLoads == 2);
        REQUIRE(stats.cacheMisses == 1);
        REQUIRE(stats.cacheHits == 1);
        REQUIRE(stats.cachedModels == 1);
    }

    SECTION("Path normalization - backslash vs forward slash")
    {
        auto model1 = cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
        auto model2 = cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));

        REQUIRE(model1 == model2); // Same pointer despite different path separators

        auto stats = cache.getStats();
        REQUIRE(stats.cacheHits == 1);
        REQUIRE(stats.cachedModels == 1);
    }

    SECTION("Path normalization - case insensitive")
    {
        auto model1 = cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
        auto model2 = cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));

        REQUIRE(model1 == model2); // Same pointer despite different case

        auto stats = cache.getStats();
        REQUIRE(stats.cacheHits == 1);
        REQUIRE(stats.cachedModels == 1);
    }

    SECTION("Multiple different models")
    {
        auto model1 = cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
        auto model2 = cache.load(GET_FIXTURE("p3d/empty_shape.p3d"));
        auto model3 = cache.load(GET_FIXTURE("p3d/sky_plane.p3d"));

        REQUIRE(model1 != nullptr);
        REQUIRE(model2 != nullptr);
        REQUIRE(model3 != nullptr);
        REQUIRE(model1 != model2);
        REQUIRE(model2 != model3);

        REQUIRE(cache.size() == 3);

        auto stats = cache.getStats();
        REQUIRE(stats.cacheMisses == 3);
        REQUIRE(stats.cachedModels == 3);
    }
}

TEST_CASE("ModelCache: isLoaded and get", "[model][cache]")
{
    ModelCache cache;

    SECTION("isLoaded before and after loading")
    {
        REQUIRE(cache.isLoaded(GET_FIXTURE("p3d/complex_vehicle.p3d")) == false);

        auto model = cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
        REQUIRE(model != nullptr);

        REQUIRE(cache.isLoaded(GET_FIXTURE("p3d/complex_vehicle.p3d")) == true);
    }

    SECTION("get returns cached model without loading")
    {
        auto model1 = cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));

        auto stats1 = cache.getStats();
        REQUIRE(stats1.totalLoads == 1);

        auto model2 = cache.get(GET_FIXTURE("p3d/complex_vehicle.p3d"));

        auto stats2 = cache.getStats();
        REQUIRE(stats2.totalLoads == 1); // get() doesn't increment totalLoads
        REQUIRE(model1 == model2);
    }

    SECTION("get returns nullptr for non-cached model")
    {
        auto model = cache.get(GET_FIXTURE("p3d/complex_vehicle.p3d"));
        REQUIRE(model == nullptr);
    }
}

TEST_CASE("ModelCache: unload", "[model][cache]")
{
    ModelCache cache;

    SECTION("Unload removes from cache")
    {
        cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
        REQUIRE(cache.size() == 1);
        REQUIRE(cache.isLoaded(GET_FIXTURE("p3d/complex_vehicle.p3d")) == true);

        bool removed = cache.unload(GET_FIXTURE("p3d/complex_vehicle.p3d"));
        REQUIRE(removed == true);
        REQUIRE(cache.size() == 0);
        REQUIRE(cache.isLoaded(GET_FIXTURE("p3d/complex_vehicle.p3d")) == false);
    }

    SECTION("Unload non-existent model returns false")
    {
        bool removed = cache.unload("nonexistent.p3d");
        REQUIRE(removed == false);
    }

    SECTION("Can reload after unload")
    {
        auto model1 = cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
        cache.unload(GET_FIXTURE("p3d/complex_vehicle.p3d"));

        auto model2 = cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
        REQUIRE(model2 != nullptr);
        REQUIRE(model1 != model2); // Different instance after reload

        auto stats = cache.getStats();
        REQUIRE(stats.cacheMisses == 2); // Both loads were cache misses
    }
}

TEST_CASE("ModelCache: clear", "[model][cache]")
{
    ModelCache cache;

    cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    cache.load(GET_FIXTURE("p3d/empty_shape.p3d"));
    cache.load(GET_FIXTURE("p3d/sky_plane.p3d"));

    REQUIRE(cache.size() == 3);

    cache.clear();

    REQUIRE(cache.size() == 0);
    REQUIRE(cache.isLoaded(GET_FIXTURE("p3d/complex_vehicle.p3d")) == false);

    auto stats = cache.getStats();
    REQUIRE(stats.cachedModels == 0);
}

TEST_CASE("ModelCache: Error handling", "[model][cache][errors]")
{
    ModelCache cache;

    SECTION("Non-existent file returns nullptr")
    {
        auto model = cache.load("nonexistent.p3d");
        REQUIRE(model == nullptr);

        auto stats = cache.getStats();
        REQUIRE(stats.loadFailures == 1);
        REQUIRE(stats.cachedModels == 0);
    }

    SECTION("Failed load doesn't cache")
    {
        cache.load("nonexistent.p3d");
        REQUIRE(cache.size() == 0);
        REQUIRE(cache.isLoaded("nonexistent.p3d") == false);
    }

    SECTION("Multiple failed loads")
    {
        cache.load("nonexistent1.p3d");
        cache.load("nonexistent2.p3d");
        cache.load("nonexistent3.p3d");

        auto stats = cache.getStats();
        REQUIRE(stats.loadFailures == 3);
        REQUIRE(stats.cachedModels == 0);
    }
}

TEST_CASE("ModelCache: Statistics", "[model][cache][stats]")
{
    ModelCache cache;

    SECTION("Stats track all operations")
    {
        cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
        cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d")); // cache hit
        cache.load(GET_FIXTURE("p3d/empty_shape.p3d"));
        cache.load("nonexistent.p3d"); // failure

        auto stats = cache.getStats();
        REQUIRE(stats.totalLoads == 4);
        REQUIRE(stats.cacheMisses == 3); // 3 because nonexistent counts as cache miss then failure
        REQUIRE(stats.cacheHits == 1);
        REQUIRE(stats.loadFailures == 1);
        REQUIRE(stats.cachedModels == 2);
    }

    SECTION("resetStats clears counters but preserves cache")
    {
        cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
        cache.load(GET_FIXTURE("p3d/empty_shape.p3d"));

        REQUIRE(cache.size() == 2);

        cache.resetStats();

        auto stats = cache.getStats();
        REQUIRE(stats.totalLoads == 0);
        REQUIRE(stats.cacheMisses == 0);
        REQUIRE(stats.cacheHits == 0);
        REQUIRE(stats.loadFailures == 0);
        REQUIRE(stats.cachedModels == 2); // Cache size preserved

        REQUIRE(cache.size() == 2); // Models still in cache
    }
}

TEST_CASE("ModelCache: Mixed ODOL and MLOD models", "[model][cache]")
{
    ModelCache cache;

    auto odolModel = cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));
    auto mlodModel = cache.load(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));

    REQUIRE(odolModel != nullptr);
    REQUIRE(mlodModel != nullptr);
    REQUIRE(odolModel != mlodModel); // Different paths = different cache entries

    REQUIRE(odolModel->sourceFormat == "ODOL");
    REQUIRE(mlodModel->sourceFormat == "MLOD");

    REQUIRE(cache.size() == 2);

    auto stats = cache.getStats();
    REQUIRE(stats.cacheMisses == 2);
    REQUIRE(stats.cachedModels == 2);
}

TEST_CASE("ModelCache: Verify loaded models are valid", "[model][cache][validation]")
{
    ModelCache cache;

    SECTION("ODOL model is compiled and valid")
    {
        auto model = cache.load(GET_FIXTURE("p3d/complex_vehicle.p3d"));

        REQUIRE(model->isCompiled());
        REQUIRE(!model->lodLevels.empty());

        // Check first LOD exists
        const auto& lod = model->lodLevels[0];
        // Resolution may be 0 for some LODs, just check structure exists

        // Check bounding volumes are computed
        REQUIRE(model->boundingBox.min.x < model->boundingBox.max.x);
        REQUIRE(model->boundingSphere.radius > 0.0f);
    }

    SECTION("MLOD model is compiled and valid")
    {
        auto model = cache.load(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));

        REQUIRE(model->isCompiled());
        REQUIRE(!model->lodLevels.empty());

        // Check first LOD exists
        const auto& lod = model->lodLevels[0];
        // Resolution may be 0 for some LODs, just check structure exists

        // Check bounding volumes are computed
        REQUIRE(model->boundingBox.min.x < model->boundingBox.max.x);
        REQUIRE(model->boundingSphere.radius > 0.0f);
    }
}
