#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Graphics/Rendering/RenderPassDescriptor.hpp>

// Phase 2.1: pin the defaults and equality semantics of
// `RenderPassDescriptor`.  Subsequent Phase 2.2 tests will exercise the
// `BuildRenderPassDescriptor` translation function; for now we just want
// the type to exist with sensible defaults and grep-able enum values.

using namespace Poseidon::render;

TEST_CASE("RenderPassDescriptor: default state is opaque world geometry", "[render-descriptor][phase2]")
{
    const RenderPassDescriptor d;
    REQUIRE(d.pass == PassKind::WorldOpaque);
    REQUIRE(d.depth == DepthMode::Normal);
    REQUIRE(d.blend == BlendMode::Opaque);
    REQUIRE(d.fog == FogMode::Enabled);
    REQUIRE(d.cull == CullMode::Back);
    REQUIRE(d.frontFace == FrontFaceMode::CW);
    REQUIRE(d.alpha == AlphaMode::Disabled);
    REQUIRE(d.lighting == LightingMode::Lit);
    REQUIRE(d.texGen == TexGenMode::None);
    REQUIRE(d.surface == SurfaceMode::Default);
    REQUIRE(d.sampler.filter == SamplerFilter::Linear);
    REQUIRE(d.sampler.clampU == false);
    REQUIRE(d.sampler.clampV == false);
    REQUIRE(d.shader == ShaderFamily::Normal);
    REQUIRE(d.alphaRef == 0);
    REQUIRE(d.stencilExclusion == false);
}

TEST_CASE("RenderPassDescriptor: equality operator", "[render-descriptor][phase2]")
{
    RenderPassDescriptor a;
    RenderPassDescriptor b;
    REQUIRE(a == b);

    SECTION("PassKind diff breaks equality")
    {
        b.pass = PassKind::CockpitOpaque;
        REQUIRE(a != b);
    }
    SECTION("FogMode diff breaks equality")
    {
        b.fog = FogMode::Disabled;
        REQUIRE(a != b);
    }
    SECTION("SamplerMode diff breaks equality")
    {
        b.sampler.clampU = true;
        REQUIRE(a != b);
    }
    SECTION("alphaRef diff breaks equality")
    {
        b.alphaRef = 0x80;
        REQUIRE(a != b);
    }
    SECTION("stencilExclusion diff breaks equality")
    {
        b.stencilExclusion = true;
        REQUIRE(a != b);
    }
}

TEST_CASE("RenderPassDescriptor: pass kinds are distinct", "[render-descriptor][phase2]")
{
    // Spot-check the documented routing axes — the descriptor's whole
    // purpose is to let downstream code branch on PassKind explicitly.
    REQUIRE(PassKind::WorldOpaque != PassKind::CockpitOpaque);
    REQUIRE(PassKind::WorldTransparent != PassKind::CockpitTransparent);
    REQUIRE(PassKind::SurfaceOverlay != PassKind::ScreenSpace3D);
    REQUIRE(PassKind::WorldShadow != PassKind::WorldOpaque);
}

TEST_CASE("RenderPassDescriptor: cockpit-like descriptor mutation", "[render-descriptor][phase2]")
{
    // Construct a representative "cockpit interior alpha-cutout" draw
    // and verify all the cockpit-pass invariants the descriptor is
    // supposed to enforce in one place.
    RenderPassDescriptor d;
    d.pass = PassKind::CockpitCutout;
    d.fog = FogMode::Disabled;
    d.lighting = LightingMode::SunDisabled;
    d.alpha = AlphaMode::Test;
    d.alphaRef = 0xc0;

    REQUIRE(d.pass == PassKind::CockpitCutout);
    REQUIRE(d.fog == FogMode::Disabled);
    REQUIRE(d.lighting == LightingMode::SunDisabled);
    REQUIRE(d.alpha == AlphaMode::Test);
    REQUIRE(d.alphaRef == 0xc0);

    // Defaults the cockpit descriptor inherits — depth normal, cull back,
    // CW winding, opaque blend.  If a future refactor changes the
    // descriptor's defaults this test fails loudly so the cockpit-
    // invariant assumption is re-examined.
    REQUIRE(d.depth == DepthMode::Normal);
    REQUIRE(d.cull == CullMode::Back);
    REQUIRE(d.frontFace == FrontFaceMode::CW);
    REQUIRE(d.blend == BlendMode::Opaque);
}

TEST_CASE("RenderPassDescriptor: shadow descriptor", "[render-descriptor][phase2]")
{
    RenderPassDescriptor d;
    d.pass = PassKind::WorldShadow;
    d.depth = DepthMode::Shadow;
    d.blend = BlendMode::Shadow;
    d.fog = FogMode::Disabled;
    d.shader = ShaderFamily::Shadow;
    d.stencilExclusion = true;

    REQUIRE(d.depth == DepthMode::Shadow);
    REQUIRE(d.blend == BlendMode::Shadow);
    REQUIRE(d.shader == ShaderFamily::Shadow);
    REQUIRE(d.stencilExclusion);
}

TEST_CASE("RenderPassDescriptor: sampler clamp is per-axis", "[render-descriptor][phase2]")
{
    SamplerMode s;
    REQUIRE(s.clampU == false);
    REQUIRE(s.clampV == false);

    s.clampU = true;
    REQUIRE(s.clampU);
    REQUIRE_FALSE(s.clampV);

    SamplerMode t;
    t.clampU = true;
    REQUIRE(s == t);

    t.filter = SamplerFilter::Point;
    REQUIRE(s != t);
}
