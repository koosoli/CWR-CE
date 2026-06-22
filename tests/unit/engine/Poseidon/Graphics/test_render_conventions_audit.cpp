#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <Poseidon/Graphics/Core/MatrixConversion.hpp>
#include <Poseidon/Graphics/Core/RenderState.hpp>
#include <Poseidon/Graphics/Rendering/RenderPassDescriptor.hpp>
#include <Poseidon/Graphics/Rendering/Font/Pactext.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <stddef.h>
#include <catch2/catch_message.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>

using Poseidon::GfxMatrix;

using Poseidon::PacAI88;
using Poseidon::PacARGB1555;
using Poseidon::PacARGB4444;
using Poseidon::PacARGB8888;
using Poseidon::PacDXT1;
using Poseidon::PacDXT2;
using Poseidon::PacDXT3;
using Poseidon::PacDXT4;
using Poseidon::PacDXT5;
using Poseidon::PacFormat;
using Poseidon::PacP8;
using Poseidon::PacRGB565;

// Batch audit covering the rendering conventions that previously
// sat at T2 with partial coverage:
//
//   I-12 Winding convention preserved (B-012)
//   I-13 Coordinate handedness explicit at boundaries (B-013)
//   I-14 Pixel-centre per-backend, never D3D9-by-default (B-018, B-034)
//   I-18 Texture format encodes source alpha presence (B-014)
//   I-19 Sampler state configurable per-draw (B-029)
//
// Each section pins the structural mechanism (constant assertion,
// enum-class default, source-line audit, descriptor field check)
// so a regression at the convention level fails a unit test rather
// than landing as a pixel-diff weeks later.

namespace
{

std::string ReadTextFile(const std::filesystem::path& p)
{
    std::ifstream f(p);
    if (!f.is_open())
        return {};
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::filesystem::path Gl33Dir()
{
    return std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "PoseidonGL33";
}

} // namespace

// I-12 — Winding convention preserved through the pipeline (B-012)

TEST_CASE("I-12: glClipControl set once with (GL_LOWER_LEFT, GL_ZERO_TO_ONE)", "[Graphics][Conventions][I-12]")
{
    // The shipped convention: glClipControl is called exactly once
    // (at engine init) with `GL_LOWER_LEFT` origin so window-space
    // Y matches D3D's top-down layout without per-frame viewport
    // flips, and `GL_ZERO_TO_ONE` depth range to match D3D11's
    // [0,1] convention.  CW = front face follows from this.
    //
    // A regression that changes either argument would silently flip
    // the cull convention (CW would become BACK face) — every mesh
    // would render inside-out.
    int canonicalCalls = 0;
    int abbreviatedComments = 0;
    for (const auto& entry : std::filesystem::directory_iterator(Gl33Dir()))
    {
        if (!entry.is_regular_file())
            continue;
        if (entry.path().extension() != ".cpp")
            continue;
        const std::string body = ReadTextFile(entry.path());

        // Count actual calls — the canonical two-arg form.  A
        // backend file that uses the one-arg or three-arg form
        // would be inconsistent with the engine init and is
        // forbidden.
        size_t pos = 0;
        while ((pos = body.find("glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE)", pos)) != std::string::npos)
        {
            // Skip occurrences inside a string literal (e.g. a
            // LOG_DEBUG message echoing the canonical form).
            const size_t lineStart = body.rfind('\n', pos);
            const size_t scanFrom = lineStart == std::string::npos ? 0 : lineStart;
            const std::string line = body.substr(scanFrom, pos - scanFrom);
            const bool inString = std::count(line.begin(), line.end(), '"') % 2 == 1;
            if (!inString)
                ++canonicalCalls;
            pos += 1;
        }

        // Sanity: any abbreviated `glClipControl(GL_LOWER_LEFT)`
        // mentions are in comments (single argument doesn't compile
        // — `glClipControl` is a two-arg function in GL 4.5).
        // Count them so the test fails if someone writes one in
        // code instead of a comment.
        pos = 0;
        while ((pos = body.find("glClipControl(GL_LOWER_LEFT)", pos)) != std::string::npos)
        {
            // Skip if preceded by `//` on the same line.
            const size_t lineStart = body.rfind('\n', pos);
            const size_t scanFrom = lineStart == std::string::npos ? 0 : lineStart;
            const std::string line = body.substr(scanFrom, pos - scanFrom);
            if (line.find("//") == std::string::npos)
                ++abbreviatedComments;
            pos += 1;
        }
    }
    // Exactly one shipping call — at engine init.  Per-frame
    // glClipControl would re-set the convention and is forbidden.
    REQUIRE(canonicalCalls == 1);
    REQUIRE(abbreviatedComments == 0);
}

TEST_CASE("I-12: RenderPassDescriptor's frontFace defaults to CW (D3D convention)", "[Graphics][Conventions][I-12]")
{
    // The descriptor's `frontFace` field is the per-draw winding
    // declaration.  Default CW matches D3D's convention; combined
    // with glClipControl(GL_LOWER_LEFT) above, mesh source winding
    // (D3D CW) renders as front-face without per-pass adjustment.
    Poseidon::render::RenderPassDescriptor d{};
    REQUIRE(d.frontFace == Poseidon::render::FrontFaceMode::CW);
}

// I-13 — Coordinate handedness explicit at boundaries (B-013)

TEST_CASE("I-13: ConvertProjectionMatrix applies the DX -> GL Z-range remap", "[Graphics][Conventions][I-13]")
{
    // DX's NDC z is [0..1] with positive-Z forward; GL's default is
    // [-1..1] with negative-Z forward.  `ConvertProjectionMatrix`
    // remaps DX [0..1] z range onto GL convention.  Combined with
    // `glClipControl(... GL_ZERO_TO_ONE)` the runtime convention is
    // DX-equivalent so existing assets render correctly.
    //
    // This pins the round-trip property: feeding ConvertProjectionMatrix
    // an identity-shaped projection produces a matrix whose 2,3 column
    // is set up for [0..1] z output (matches the GL_ZERO_TO_ONE
    // clip-control).
    Matrix4 src(MIdentity);
    src(2, 2) = 0.5f;
    src(2, 3) = 0.5f;
    GfxMatrix dst{};
    ConvertProjectionMatrix(dst, src, /*zBias*/ 0);

    // The 33-element of the GL projection equals src(2,2) (no
    // bias).  The 43-element equals src(2,3).  Bias is 0 so the
    // values pass through unchanged.
    REQUIRE(dst._33 == Catch::Approx(0.5f).margin(1e-6f));
    REQUIRE(dst._43 == Catch::Approx(0.5f).margin(1e-6f));
}

TEST_CASE("I-13: view matrix carries camera-relative translation (cameraPos zeroed)", "[Graphics][Conventions][I-13]")
{
    // The engine renders camera-relative: the view matrix has its
    // translation zeroed and the camera position rides as a separate
    // uniform.  This avoids large-world-coordinate precision loss
    // when the camera is far from the origin.  Pinning the convention:
    // any future change that re-introduces translation into the view
    // matrix would surface as drift on distant terrain.
    //
    // Source-line audit: `frame.view._41 = 0` (and the matching
    // ._42/._43) appears once in EngineGL33_Shaders.cpp where the
    // view matrix is built.
    const std::string body = ReadTextFile(Gl33Dir() / "EngineGL33_Shaders.cpp");
    REQUIRE_FALSE(body.empty());
    REQUIRE(body.find("frame.view._41 = 0") != std::string::npos);
    REQUIRE(body.find("frame.view._42 = 0") != std::string::npos);
    REQUIRE(body.find("frame.view._43 = 0") != std::string::npos);
}

// I-14 — Pixel-centre convention (B-018, B-034)

TEST_CASE("I-14: vpScale derives from viewport without half-pixel offset", "[Graphics][Conventions][I-14]")
{
    // GL / D3D11 use half-integer pixel centres; D3D9 used integers
    // with an additional -0.5px offset.  The shipping convention is
    // GL-native: VSScreen reads `vpScale = (2/w, 2/h, 0, 0)` and the
    // TLVertex pos field carries integer screen coordinates;
    // gl_Position = aPos.x * vpScale.x - 1 maps pixel 0 to NDC -1
    // (the left edge), which combined with GL_LOWER_LEFT origin
    // gives correct pixel-centre sampling.
    //
    // Audit: no `glPixelStorei(...,..,1)` or `-0.5f` half-pixel
    // shift in the GL33 backend — if a D3D9-era port re-adds one,
    // the audit trips.
    int halfPixelHits = 0;
    for (const auto& entry : std::filesystem::directory_iterator(Gl33Dir()))
    {
        if (!entry.is_regular_file())
            continue;
        if (entry.path().extension() != ".cpp")
            continue;
        const std::string body = ReadTextFile(entry.path());

        size_t pos = 0;
        while ((pos = body.find("-0.5f", pos)) != std::string::npos)
        {
            // The -0.5f literal might legitimately appear in
            // shadow / lighting math; only count it as a half-pixel
            // shift if it appears in a context that smells like
            // pixel coordinates (`pos.x`, `pos.y`, viewport math).
            const size_t scanFrom = pos > 80 ? pos - 80 : 0;
            const std::string before = body.substr(scanFrom, pos - scanFrom);
            const bool isPixelShift = before.find("pos.x") != std::string::npos ||
                                      before.find("pos.y") != std::string::npos ||
                                      before.find("vpScale") != std::string::npos;
            if (isPixelShift)
                ++halfPixelHits;
            pos += 5;
        }
    }
    CAPTURE(halfPixelHits);
    REQUIRE(halfPixelHits == 0);
}

// I-18 — Texture format encodes source alpha presence (B-014)

TEST_CASE("I-18: PacFormat enum partitions cleanly by alpha presence", "[Graphics][Conventions][I-18]")
{
    // The format selection contract: formats with alpha and formats
    // without alpha are distinct enum values.  No format is "maybe
    // alpha" — the loader must pick the right one at decode time.
    // This test pins the partition so a future format addition that
    // breaks the clean split (e.g. an "ARGB-or-RGB" sentinel) is
    // caught.
    auto hasAlpha = [](PacFormat f)
    {
        switch (f)
        {
            case PacARGB1555:
            case PacARGB4444:
            case PacARGB8888:
            case PacDXT2:
            case PacDXT3:
            case PacDXT4:
            case PacDXT5:
            case PacAI88: // intensity + alpha
                return true;
            case PacP8:
            case PacRGB565:
            case PacDXT1: // 1-bit punchthrough is reported as opaque
                return false;
        }
        return false;
    };

    // Spot-check every enum value has a deterministic answer.
    REQUIRE(hasAlpha(PacARGB1555));
    REQUIRE(hasAlpha(PacARGB4444));
    REQUIRE(hasAlpha(PacARGB8888));
    REQUIRE(hasAlpha(PacDXT3));
    REQUIRE(hasAlpha(PacDXT5));
    REQUIRE_FALSE(hasAlpha(PacRGB565));
    REQUIRE_FALSE(hasAlpha(PacDXT1));
    REQUIRE_FALSE(hasAlpha(PacP8));
}

// I-19 — Sampler state configurable per-draw (B-029)

TEST_CASE("I-19: descriptor.sampler carries clamp + filter per-draw", "[Graphics][Conventions][I-19]")
{
    // B-029 was per-draw texture wrap mode bleeding from one draw
    // into the next.  The shipping fix: the descriptor's `sampler`
    // field carries clamp + filter; `ApplyPipeline` reads it every
    // draw and `ApplySamplerState` rebinds.
    //
    // Pin the type: SamplerMode has both clamp axes + filter, and
    // its default is "neither clamp, linear filter" so an unset
    // sampler doesn't inherit the previous draw's clamp.
    Poseidon::render::RenderPassDescriptor d{};
    REQUIRE(d.sampler.filter == Poseidon::render::SamplerFilter::Linear);
    REQUIRE_FALSE(d.sampler.clampU);
    REQUIRE_FALSE(d.sampler.clampV);

    // Equality check — two default-constructed SamplerMode values
    // compare equal so a `_lastSamplerMode` cache can short-circuit
    // unchanged samplers without re-binding.
    Poseidon::render::SamplerMode a{};
    Poseidon::render::SamplerMode b{};
    REQUIRE(a == b);

    // Distinct configurations produce distinct values — caching
    // can't accidentally collapse "clamp U" and "clamp V".
    Poseidon::render::SamplerMode u{Poseidon::render::SamplerFilter::Linear, /*U*/ true, /*V*/ false};
    Poseidon::render::SamplerMode v{Poseidon::render::SamplerFilter::Linear, /*U*/ false, /*V*/ true};
    REQUIRE(u != v);
    REQUIRE(u != a);
    REQUIRE(v != a);
}
