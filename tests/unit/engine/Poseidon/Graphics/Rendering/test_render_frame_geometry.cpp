#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Graphics/Rendering/Frame/Geometry.hpp>

#include <cstdint>
#include <vector>

// Phase E.1 — pin the fan-triangulation contract (invariant I-10,
// catalog B-026).  The historical bug: `MeshGL::DrawPolygon` called
// `glDrawElements(GL_TRIANGLES, n)` for n-vertex polygon, producing
// `n/3` triangles instead of `n-2`.  Quads rendered as one triangle.
//
// The fix is now a pure function — these tests pin the math so the
// next backend (or the next refactor) inherits the contract.

namespace v2 = Poseidon::render::frame;

TEST_CASE("Frame/I-10: FanTriangleCount returns n-2 for n>=3, 0 below", "[render-frame][invariants][I-10][geometry]")
{
    REQUIRE(Poseidon::render::frame::FanTriangleCount(-1) == 0);
    REQUIRE(Poseidon::render::frame::FanTriangleCount(0) == 0);
    REQUIRE(Poseidon::render::frame::FanTriangleCount(1) == 0);
    REQUIRE(Poseidon::render::frame::FanTriangleCount(2) == 0);
    REQUIRE(Poseidon::render::frame::FanTriangleCount(3) == 1);
    REQUIRE(Poseidon::render::frame::FanTriangleCount(4) == 2);
    REQUIRE(Poseidon::render::frame::FanTriangleCount(5) == 3);
    REQUIRE(Poseidon::render::frame::FanTriangleCount(8) == 6);
    REQUIRE(Poseidon::render::frame::FanTriangleCount(16) == 14);
}

TEST_CASE("Frame/I-10: FanIndexCount is 3 * triangle count", "[render-frame][invariants][I-10][geometry]")
{
    REQUIRE(Poseidon::render::frame::FanIndexCount(3) == 3);
    REQUIRE(Poseidon::render::frame::FanIndexCount(4) == 6);
    REQUIRE(Poseidon::render::frame::FanIndexCount(8) == 18);
}

TEST_CASE("Frame/I-10: AppendFanIndices for quad produces (0,1,2, 0,2,3)", "[render-frame][invariants][I-10][geometry]")
{
    std::vector<std::uint16_t> indices;
    const int produced = Poseidon::render::frame::AppendFanIndices(indices, 4);

    REQUIRE(produced == 6);
    REQUIRE(indices.size() == 6);
    REQUIRE(indices[0] == 0);
    REQUIRE(indices[1] == 1);
    REQUIRE(indices[2] == 2);
    REQUIRE(indices[3] == 0);
    REQUIRE(indices[4] == 2);
    REQUIRE(indices[5] == 3);
}

TEST_CASE("Frame/I-10: AppendFanIndices respects baseVertex offset", "[render-frame][invariants][I-10][geometry]")
{
    std::vector<std::uint16_t> indices;
    Poseidon::render::frame::AppendFanIndices(indices, 4, /*baseVertex=*/100);

    REQUIRE(indices[0] == 100);
    REQUIRE(indices[1] == 101);
    REQUIRE(indices[2] == 102);
    REQUIRE(indices[3] == 100);
    REQUIRE(indices[4] == 102);
    REQUIRE(indices[5] == 103);
}

TEST_CASE("Frame/I-10: AppendFanIndices for hexagon produces 4 triangles", "[render-frame][invariants][I-10][geometry]")
{
    std::vector<std::uint16_t> indices;
    const int produced = Poseidon::render::frame::AppendFanIndices(indices, 6);

    // Hexagon → 4 triangles: (0,1,2), (0,2,3), (0,3,4), (0,4,5).
    REQUIRE(produced == 12);
    REQUIRE(indices.size() == 12);

    // Pivot is always vertex 0.
    for (size_t i = 0; i < indices.size(); i += 3)
        REQUIRE(indices[i] == 0);

    // Last triangle ends at last vertex.
    REQUIRE(indices.back() == 5);
}

TEST_CASE("Frame/I-10: AppendFanIndices preserves existing contents", "[render-frame][invariants][I-10][geometry]")
{
    std::vector<std::uint16_t> indices = {42, 43, 44};
    Poseidon::render::frame::AppendFanIndices(indices, 3);

    REQUIRE(indices.size() == 6);
    REQUIRE(indices[0] == 42);
    REQUIRE(indices[1] == 43);
    REQUIRE(indices[2] == 44);
    REQUIRE(indices[3] == 0);
    REQUIRE(indices[4] == 1);
    REQUIRE(indices[5] == 2);
}
