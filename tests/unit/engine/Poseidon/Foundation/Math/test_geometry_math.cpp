#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

// vecTempl.hpp requires permissive mode for full template math - test via math3d
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>

TEST_CASE("vecTempl: Vector3 dot product via math3d", "[Math]")
{
    Vector3 a(1.0f, 2.0f, 3.0f);
    Vector3 b(4.0f, 5.0f, 6.0f);
    REQUIRE(a * b == Catch::Approx(32.0f));
}

TEST_CASE("vecTempl: Vector3 square size", "[Math]")
{
    Vector3 v(3.0f, 4.0f, 0.0f);
    REQUIRE(v.SquareSize() == Catch::Approx(25.0f));
}

// Tier 3: compile checks
#include <Poseidon/Foundation/Memory/Heap.hpp>
TEST_CASE("heap.hpp compiles", "[Math]")
{
    REQUIRE(true);
}

TEST_CASE("sortTree.hpp: placeholder", "[Math]")
{
    REQUIRE(true);
}

#include <Poseidon/Foundation/Math/Quatrix.hpp>
TEST_CASE("quatrix.hpp compiles", "[Math]")
{
    Quatrix4 q4;
    Quatrix3 q3;
    REQUIRE(sizeof(q4) > 0);
    REQUIRE(sizeof(q3) > 0);
}

#include <Poseidon/Core/HandledList.hpp>
TEST_CASE("handledList.hpp compiles", "[Math]")
{
    REQUIRE(true);
}

#include <Poseidon/Foundation/Math/PolyClip.hpp>
TEST_CASE("polyClip.hpp compiles", "[Math]")
{
    REQUIRE(true);
}

#include <Poseidon/Foundation/Math/V3Quads.hpp>
TEST_CASE("v3quads.hpp compiles", "[Math]")
{
    REQUIRE(true);
}
