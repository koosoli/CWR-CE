#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Graphics/Rendering/Frame/Coordinates.hpp>

// Phase E.3 - coordinate-convention invariants.  Three catalog bug
// classes addressed via pure constexpr helpers + golden-input
// tests:
//
//   I-12 / B-012  winding convention preserved end to end.
//   I-13 / B-013  coordinate handedness explicit at conversions.
//   I-14 / B-018, B-034  pixel-centre per-backend, never D3D9-default.
//
// No GL ctx, no fixture chain, no engine init.

namespace v2 = Poseidon::render::frame;

// I-12 / B-012 - Winding

TEST_CASE("Frame/I-12: ComputeWinding2D - CW triangle in Y-down screen space",
          "[render-frame][invariants][I-12][coordinates]")
{
    // In screen space (Y down), the triangle (0,0)-(10,0)-(0,10)
    // traverses right, then down - clockwise.
    REQUIRE(Poseidon::render::frame::ComputeWinding2D(0, 0, 10, 0, 0, 10) == Poseidon::render::frame::WindingOrder::CW);
}

TEST_CASE("Frame/I-12: ComputeWinding2D - CCW triangle in Y-down screen space",
          "[render-frame][invariants][I-12][coordinates]")
{
    // Reverse the second and third vertex → CCW.
    REQUIRE(Poseidon::render::frame::ComputeWinding2D(0, 0, 0, 10, 10, 0) ==
            Poseidon::render::frame::WindingOrder::CCW);
}

TEST_CASE("Frame/I-12: ComputeWinding2D - degenerate (colinear) classifies CW",
          "[render-frame][invariants][I-12][coordinates]")
{
    // Colinear → cross product 0 → CW (per the helper's contract).
    REQUIRE(Poseidon::render::frame::ComputeWinding2D(0, 0, 10, 0, 20, 0) == Poseidon::render::frame::WindingOrder::CW);
}

TEST_CASE("Frame/I-12: kFrontFaceWinding is CW", "[render-frame][invariants][I-12][coordinates]")
{
    // the frame layer's commitment: mesh source CW, rasterizer CW, no Y-flip.
    // If a future change drifts this to CCW, every CW mesh asset
    // would back-face cull - exactly B-012.
    static_assert(Poseidon::render::frame::kFrontFaceWinding == Poseidon::render::frame::WindingOrder::CW);
    REQUIRE(Poseidon::render::frame::kFrontFaceWinding == Poseidon::render::frame::WindingOrder::CW);
}

// I-13 / B-013 - Handedness

TEST_CASE("Frame/I-13: kViewSpaceHandedness is RightHanded (GL convention)",
          "[render-frame][invariants][I-13][coordinates]")
{
    static_assert(Poseidon::render::frame::kViewSpaceHandedness == Poseidon::render::frame::Handedness::RightHanded);
    REQUIRE(Poseidon::render::frame::kViewSpaceHandedness == Poseidon::render::frame::Handedness::RightHanded);
}

TEST_CASE("Frame/I-13: ViewForwardZSign is -1 for RightHanded, +1 for LeftHanded",
          "[render-frame][invariants][I-13][coordinates]")
{
    // The B-013 failure: a left-handed view matrix produced
    // positive view-space Z for points in front of the camera,
    // mirroring everything.  The sign here is the golden contract.
    static_assert(Poseidon::render::frame::ViewForwardZSign(Poseidon::render::frame::Handedness::RightHanded) == -1.0f);
    static_assert(Poseidon::render::frame::ViewForwardZSign(Poseidon::render::frame::Handedness::LeftHanded) == +1.0f);

    // Default argument uses kViewSpaceHandedness.
    REQUIRE(Poseidon::render::frame::ViewForwardZSign() == -1.0f);
}

// I-14 / B-018, B-034 - Pixel centre

TEST_CASE("Frame/I-14: kPixelCentre is HalfInteger (modern backends)", "[render-frame][invariants][I-14][coordinates]")
{
    static_assert(Poseidon::render::frame::kPixelCentre == Poseidon::render::frame::PixelCentre::HalfInteger);
    REQUIRE(Poseidon::render::frame::kPixelCentre == Poseidon::render::frame::PixelCentre::HalfInteger);
}

TEST_CASE("Frame/I-14: PixelCentreOffset is 0 for HalfInteger, 0.5 for Integer",
          "[render-frame][invariants][I-14][coordinates]")
{
    // The B-018 / B-034 failure: code applied 0.5 (D3D9 offset)
    // on a HalfInteger backend, producing the screen-top hairline
    // and vector-font top-clipping.  The constant is now typed.
    static_assert(Poseidon::render::frame::PixelCentreOffset(Poseidon::render::frame::PixelCentre::HalfInteger) ==
                  0.0f);
    static_assert(Poseidon::render::frame::PixelCentreOffset(Poseidon::render::frame::PixelCentre::Integer) == 0.5f);

    // Default argument uses kPixelCentre.
    REQUIRE(Poseidon::render::frame::PixelCentreOffset() == 0.0f);
}

TEST_CASE("Frame/I-14: applying default offset to integer pixel position is identity",
          "[render-frame][invariants][I-14][coordinates]")
{
    // The intended use: caller takes integer pixel coord, adds
    // PixelCentreOffset to land on rasterizer's sample point.
    // On HalfInteger backends this is a no-op; D3D9 it's 0.5.
    constexpr float pixelX = 100.0f;
    constexpr float adjusted = pixelX + Poseidon::render::frame::PixelCentreOffset();
    static_assert(adjusted == 100.0f);
    REQUIRE(adjusted == 100.0f);
}
