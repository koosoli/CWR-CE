#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Graphics/Core/FanDecompose.hpp>

#include <array>
#include <cstdint>

using Poseidon::render::geom::FanToTriangles;
using Poseidon::render::geom::FanTriangleIndexCount;

// I-10: polygon-fan -> triangle-list decomposition produces exactly
// `(n - 2) * 3` indices and lays them out so consecutive triangles
// share the centre vertex and the previous edge.  B-026 was the
// "quad rendered as 1 triangle" bug — root cause was an off-by-one
// in the same loop these tests now lock down.

TEST_CASE("FanTriangleIndexCount returns (n-2)*3 for n >= 3", "[Graphics][FanDecompose][I-10]")
{
    REQUIRE(FanTriangleIndexCount(3) == 3);   // triangle  -> 1 tri  -> 3 indices
    REQUIRE(FanTriangleIndexCount(4) == 6);   // quad      -> 2 tris -> 6 indices
    REQUIRE(FanTriangleIndexCount(5) == 9);   // pentagon  -> 3 tris -> 9 indices
    REQUIRE(FanTriangleIndexCount(6) == 12);  // hexagon   -> 4 tris -> 12 indices
    REQUIRE(FanTriangleIndexCount(8) == 18);  // octagon   -> 6 tris -> 18 indices
    REQUIRE(FanTriangleIndexCount(16) == 42); // 16-gon    -> 14 tris -> 42 indices
}

TEST_CASE("FanTriangleIndexCount returns 0 for degenerate input", "[Graphics][FanDecompose][I-10]")
{
    REQUIRE(FanTriangleIndexCount(0) == 0);
    REQUIRE(FanTriangleIndexCount(1) == 0);
    REQUIRE(FanTriangleIndexCount(2) == 0);
    REQUIRE(FanTriangleIndexCount(-1) == 0);
}

TEST_CASE("FanToTriangles: triangle (n=3) emits one triangle in source order", "[Graphics][FanDecompose][I-10]")
{
    const std::array<uint16_t, 3> src{10, 20, 30};
    std::array<uint16_t, 3> dst{};
    FanToTriangles(src.data(), 3, /*vertexBase*/ 0, dst.data());
    REQUIRE(dst == std::array<uint16_t, 3>{10, 20, 30});
}

TEST_CASE("FanToTriangles: quad (n=4) emits two triangles, not one (B-026)", "[Graphics][FanDecompose][I-10]")
{
    // The B-026 broken state emitted only 3 indices (one triangle) for
    // a quad; the fix must produce 6 (two triangles).  Both triangles
    // share index 0; the second triangle picks up the previous edge.
    const std::array<uint16_t, 4> src{0, 1, 2, 3};
    std::array<uint16_t, 6> dst{};
    FanToTriangles(src.data(), 4, /*vertexBase*/ 0, dst.data());
    REQUIRE(dst == std::array<uint16_t, 6>{0, 1, 2, 0, 2, 3});
}

TEST_CASE("FanToTriangles: pentagon (n=5) emits three triangles, fan-shared", "[Graphics][FanDecompose][I-10]")
{
    const std::array<uint16_t, 5> src{0, 1, 2, 3, 4};
    std::array<uint16_t, 9> dst{};
    FanToTriangles(src.data(), 5, /*vertexBase*/ 0, dst.data());
    REQUIRE(dst == std::array<uint16_t, 9>{0, 1, 2, 0, 2, 3, 0, 3, 4});
}

TEST_CASE("FanToTriangles: vertexBase offsets every emitted index", "[Graphics][FanDecompose][I-10]")
{
    const std::array<uint16_t, 4> src{0, 1, 2, 3};
    std::array<uint16_t, 6> dst{};
    FanToTriangles(src.data(), 4, /*vertexBase*/ 100, dst.data());
    REQUIRE(dst == std::array<uint16_t, 6>{100, 101, 102, 100, 102, 103});
}

TEST_CASE("FanToTriangles: degenerate inputs write nothing", "[Graphics][FanDecompose][I-10]")
{
    std::array<uint16_t, 4> dst{0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD};

    SECTION("n < 3 writes nothing")
    {
        const std::array<uint16_t, 2> src{1, 2};
        FanToTriangles(src.data(), 2, /*vertexBase*/ 0, dst.data());
        REQUIRE(dst == std::array<uint16_t, 4>{0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD});
    }

    SECTION("null src writes nothing")
    {
        FanToTriangles<uint16_t, uint16_t>(nullptr, 4, /*vertexBase*/ 0, dst.data());
        REQUIRE(dst == std::array<uint16_t, 4>{0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD});
    }
}

TEST_CASE("FanToTriangles: large fan (n=16) produces 14 triangles", "[Graphics][FanDecompose][I-10]")
{
    std::array<uint16_t, 16> src{};
    for (int i = 0; i < 16; ++i)
        src[i] = static_cast<uint16_t>(i);

    std::array<uint16_t, 42> dst{};
    FanToTriangles(src.data(), 16, /*vertexBase*/ 0, dst.data());

    // Each triangle: {0, prev, curr} for curr in [2..15]
    for (int t = 0; t < 14; ++t)
    {
        REQUIRE(dst[t * 3 + 0] == 0);
        REQUIRE(dst[t * 3 + 1] == static_cast<uint16_t>(t + 1));
        REQUIRE(dst[t * 3 + 2] == static_cast<uint16_t>(t + 2));
    }
}
