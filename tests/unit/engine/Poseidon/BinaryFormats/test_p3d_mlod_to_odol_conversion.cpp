#ifdef _M_IX86

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../test_fixtures.hpp"

#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>

using Catch::Approx;

static LODShapeWithShadow* loadMLODShape(const char* path)
{
    return new LODShapeWithShadow(path, false); // MLOD format constructor
}

TEST_CASE("MLOD->ODOL conversion in memory", "[p3d][mlod][odol][conversion]")
{
    const char* fixturePath = GET_FIXTURE("mlod/complex_vehicle_mlod.p3d");

    SECTION("Load MLOD, save as ODOL to memory, reload as ODOL")
    {
        auto* mlodShape = loadMLODShape(fixturePath);
        REQUIRE(mlodShape != nullptr);

        INFO("Loaded MLOD shape");
        int mlodLevels = mlodShape->NLevels();
        REQUIRE(mlodLevels > 0);

        QOStream ostream;
        mlodShape->SaveOptimized(ostream);

        INFO("Saved to memory as ODOL, size: " << ostream.pcount() << " bytes");
        REQUIRE(ostream.pcount() > 0);

        QIStream istream(ostream.str(), ostream.pcount());
        auto* odolShape = new LODShapeWithShadow();
        REQUIRE(odolShape->LoadOptimized(istream));

        INFO("Reloaded from memory as ODOL");

        REQUIRE(odolShape->NLevels() == mlodLevels);

        for (int i = 0; i < mlodLevels; ++i)
        {
            INFO("Comparing LOD " << i);

            const Shape* mlodLod = mlodShape->Level(i);
            const Shape* odolLod = odolShape->Level(i);

            REQUIRE(mlodLod != nullptr);
            REQUIRE(odolLod != nullptr);

            REQUIRE(mlodShape->Resolution(i) == Approx(odolShape->Resolution(i)));
            REQUIRE(mlodLod->NPoints() == odolLod->NPoints()); // MLOD compact points, ODOL expanded vertices
            REQUIRE(mlodLod->NFaces() == odolLod->NFaces());
            REQUIRE(mlodLod->NTextures() == odolLod->NTextures());
            REQUIRE(mlodLod->NNamedSel() == odolLod->NNamedSel());
        }

        REQUIRE(mlodShape->BoundingSphere() == Approx(odolShape->BoundingSphere()));
        REQUIRE(mlodShape->GeometrySphere() == Approx(odolShape->GeometrySphere()));

        delete mlodShape;
        delete odolShape;
    }
}

TEST_CASE("MLOD->ODOL->MLOD roundtrip consistency", "[p3d][mlod][odol][conversion][roundtrip]")
{
    const char* fixturePath = GET_FIXTURE("mlod/complex_vehicle_mlod.p3d");

    SECTION("Double conversion should produce identical ODOL binary")
    {
        auto* mlodShape = loadMLODShape(fixturePath);
        REQUIRE(mlodShape != nullptr);

        QOStream ostream1;
        mlodShape->SaveOptimized(ostream1);
        int size1 = ostream1.pcount();
        REQUIRE(size1 > 0);

        QIStream istream1(ostream1.str(), size1);
        auto* odolShape = new LODShapeWithShadow();
        REQUIRE(odolShape->LoadOptimized(istream1));

        QOStream ostream2;
        odolShape->SaveOptimized(ostream2);
        int size2 = ostream2.pcount();

        REQUIRE(size1 == size2);
        REQUIRE(memcmp(ostream1.str(), ostream2.str(), size1) == 0);

        delete mlodShape;
        delete odolShape;
    }
}

TEST_CASE("MLOD->ODOL conversion validation for test fixtures", "[p3d][mlod][odol][conversion][fixtures]")
{
    SECTION("complex_vehicle_mlod.p3d")
    {
        const char* fixturePath = GET_FIXTURE("mlod/complex_vehicle_mlod.p3d");

        auto* mlodShape = loadMLODShape(fixturePath);
        REQUIRE(mlodShape != nullptr);

        QOStream ostream;
        mlodShape->SaveOptimized(ostream);

        QIStream istream(ostream.str(), ostream.pcount());
        auto* odolShape = new LODShapeWithShadow();
        REQUIRE(odolShape->LoadOptimized(istream));

        // LOD 0 should have geometry
        REQUIRE(odolShape->NLevels() > 0);
        const Shape* lod0 = odolShape->Level(0);
        REQUIRE(lod0 != nullptr);
        REQUIRE(lod0->NPoints() > 0);
        REQUIRE(lod0->NFaces() > 0);

        delete mlodShape;
        delete odolShape;
    }

    SECTION("complex_vehicle_mlod.p3d repeat load")
    {
        const char* fixturePath = GET_FIXTURE("mlod/complex_vehicle_mlod.p3d");

        auto* mlodShape = loadMLODShape(fixturePath);
        REQUIRE(mlodShape != nullptr);

        QOStream ostream;
        mlodShape->SaveOptimized(ostream);

        QIStream istream(ostream.str(), ostream.pcount());
        auto* odolShape = new LODShapeWithShadow();
        REQUIRE(odolShape->LoadOptimized(istream));

        REQUIRE(odolShape->NLevels() > 0);

        delete mlodShape;
        delete odolShape;
    }

    SECTION("camera_path_a.p3d")
    {
        const char* fixturePath = GET_FIXTURE("mlod/camera_path_a.p3d");

        auto* mlodShape = loadMLODShape(fixturePath);
        REQUIRE(mlodShape != nullptr);

        QOStream ostream;
        mlodShape->SaveOptimized(ostream);

        QIStream istream(ostream.str(), ostream.pcount());
        auto* odolShape = new LODShapeWithShadow();
        REQUIRE(odolShape->LoadOptimized(istream));

        REQUIRE(odolShape->NLevels() > 0);

        delete mlodShape;
        delete odolShape;
    }
}

#endif // _M_IX86
