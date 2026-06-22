#pragma once

#include <cstdint>

// Coordinate-convention contract for the frame layer.  Pure data
// + constexpr helpers — no math library, no GL.  Single file, two
// purposes:
//
//   1.  Make the renderer's coordinate conventions *explicit* in
//       the type system.  D3D9-versus-GL handedness, CW-versus-CCW
//       winding, and integer-versus-half-integer pixel centres are all
//       conventions that bite when left implicit: CCW set against CW
//       mesh data back-face-culls everything; a left-handed view
//       matrix on a right-handed target mirrors the scene; a D3D9
//       half-pixel `(1/W, -1/H)` offset carried into GL leaves a
//       screen-top hairline of missing pixels and clips glyph tops.
//
//   2.  Provide constexpr helpers that the build can use to *verify*
//       the convention without standing up a GL context.  Tests
//       construct golden inputs and assert outputs in pure C++.

namespace Poseidon
{
namespace render::frame
{

// Winding order

// Triangle winding direction, viewed in screen space (window
// coordinates, Y pointing down — D3D-style framebuffer layout).
enum class WindingOrder : std::uint8_t
{
    CW,   // clockwise — D3D mesh-source convention; with
          // glClipControl(GL_LOWER_LEFT) this stays CW in window
          // space too, no Y-flip.
    CCW,  // counter-clockwise — default OpenGL front face if you
          // *don't* call glClipControl.
};

// The frame layer's chosen front-face winding.  Mesh source data is
// CW, the rasterizer is configured CW; the convention is preserved end
// to end.
constexpr WindingOrder kFrontFaceWinding = WindingOrder::CW;

// Compute the winding of three points in 2D screen space (Y down).
// Pure function, signed cross product of (p1-p0) × (p2-p0).
//
// In Y-down screen space the rotational sense of the cross product
// flips compared to math (Y-up) convention:
//   Positive cross product → CW  (visually clockwise on screen).
//   Negative cross product → CCW.
//   Zero cross product → degenerate; classified as CW for stability.
constexpr WindingOrder ComputeWinding2D(
    float x0, float y0,
    float x1, float y1,
    float x2, float y2)
{
    const float cross = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
    return cross < 0.0f ? WindingOrder::CCW : WindingOrder::CW;
}

// Coordinate handedness

// View / clip space handedness.
//   RightHanded — OpenGL: +X right, +Y up, -Z forward.  Camera
//                 looks down the negative Z axis.
//   LeftHanded  — D3D9/D3D11: +X right, +Y up, +Z forward.  Camera
//                 looks down the positive Z axis.
enum class Handedness : std::uint8_t
{
    RightHanded,
    LeftHanded,
};

// The frame layer commits to right-handed view space across backends.  GL is
// right-handed natively; D3D backends would need a view matrix Z
// flip applied at the boundary.
constexpr Handedness kViewSpaceHandedness = Handedness::RightHanded;

// Sign of view-space Z for a point in *front of* the camera.
// RightHanded → -1 (point is at view_z < 0).
// LeftHanded  → +1 (point is at view_z > 0).
//
// The golden-vector target for the handedness check — given the
// engine's view matrix and a known world-forward point, the
// transformed view-space Z must have this sign.
constexpr float ViewForwardZSign(Handedness h = kViewSpaceHandedness)
{
    return h == Handedness::RightHanded ? -1.0f : +1.0f;
}

// Pixel centre convention

// Where does sampling at integer pixel coordinates land relative to
// the pixel grid?
//   Integer     — D3D9 convention; pixel centre at integer
//                 coordinates.  2D drawing carried a `(+0.5, -0.5)`
//                 offset to compensate.
//   HalfInteger — GL/D3D11/D3D12/Vulkan convention; pixel centre
//                 at half-integer coordinates (e.g. pixel (0,0)
//                 centred at (0.5, 0.5)).  NO offset needed.
enum class PixelCentre : std::uint8_t
{
    Integer,
    HalfInteger,
};

// The frame layer commits to modern half-integer pixel centres.  Any
// "+0.5" fixup must therefore *not* be carried over from a D3D9 codebase.
constexpr PixelCentre kPixelCentre = PixelCentre::HalfInteger;

// Offset to apply to a 2D vertex's pixel-space coordinate to land on
// the centre of the target pixel.  For HalfInteger backends this is
// zero (the rasterizer already centres there); for Integer (D3D9)
// it's 0.5.  Applying the D3D9 0.5 on a HalfInteger backend produces a
// (+0.5px, -0.5px) shift that surfaces as a hairline of missing pixels
// at screen-top after the framebuffer Y-flip.
constexpr float PixelCentreOffset(PixelCentre c = kPixelCentre)
{
    return c == PixelCentre::Integer ? 0.5f : 0.0f;
}

} // namespace render::frame

} // namespace Poseidon
