#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Graphics/Rendering/ValidateRenderPassDescriptor.hpp>
#include <Poseidon/Graphics/Rendering/BuildRenderPassDescriptor.hpp>
#include <Poseidon/Core/Types.hpp>
#include <cstring>
#include <catch2/catch_message.hpp>

// Phase 4: validation invariants for `RenderPassDescriptor`.  Each
// test pins one descriptor invariant — a combination the design doc
// lists as impossible or suspicious — so a future refactor that
// produces a violating descriptor fails compilation- and/or test-time
// before it reaches GL state binding.

using namespace Poseidon::render;

namespace
{

bool HasError(const char* err, const char* substring)
{
    return err && std::strstr(err, substring) != nullptr;
}

} // namespace

TEST_CASE("ValidateRenderPassDescriptor: default descriptor is valid", "[validate-descriptor][phase4]")
{
    const RenderPassDescriptor d;
    REQUIRE(IsRenderPassDescriptorValid(d));
    REQUIRE(ValidateRenderPassDescriptor(d) == nullptr);
}

TEST_CASE("ValidateRenderPassDescriptor: BuildRenderPassDescriptor outputs are valid by construction",
          "[validate-descriptor][phase4]")
{
    // Every representative output of BuildRenderPassDescriptor must
    // pass validation — otherwise the translation function itself
    // produces inconsistent state.

    BuildContext ctx;

    auto check = [&](int legacyInt, BuildContext c, const char* label)
    {
        const RenderPassDescriptor d = BuildRenderPassDescriptor(SplitLegacy(legacyInt), c);
        const char* err = ValidateRenderPassDescriptor(d);
        INFO("scenario: " << label);
        INFO("validation error: " << (err ? err : "(none)"));
        REQUIRE(err == nullptr);
    };

    check(0, ctx, "default (world opaque)");
    check(NoDropdown | FogDisabled, ctx, "cockpit opaque (legacy bits)");
    check(NoDropdown | IsAlpha | FogDisabled, ctx, "cockpit transparent");
    check(IsShadow, ctx, "world shadow");
    check(IsLight, ctx, "world light");
    check(IsWater, ctx, "world water");
    check(IsAlphaFog, ctx, "world transparent (alpha-fog)");
    check(IsAlpha, ctx, "world transparent (alpha-blend)");
    check(IsTransparent, ctx, "world cutout");
    check(OnSurface, ctx, "surface overlay");
    check(NoZBuf, ctx, "sky / depth disabled");

    // Phase 3 hint-driven cockpit.  With hint set + FogDisabled bit
    // (the migration pattern from Phase 3.2), descriptor is valid.
    BuildContext cockpit = ctx;
    cockpit.passKindHint = PassKindHint::Cockpit;
    check(FogDisabled, cockpit, "hint cockpit + FogDisabled");
}

TEST_CASE("ValidateRenderPassDescriptor: cockpit pass with world fog is flagged", "[validate-descriptor][phase4]")
{
    RenderPassDescriptor d;
    d.pass = PassKind::CockpitOpaque;
    d.fog = FogMode::Enabled; // <-- should be Disabled for cockpit
    REQUIRE_FALSE(IsRenderPassDescriptorValid(d));
    REQUIRE(HasError(ValidateRenderPassDescriptor(d), "Cockpit pass with FogMode::Enabled"));
}

TEST_CASE("ValidateRenderPassDescriptor: shadow path invariants", "[validate-descriptor][phase4]")
{
    SECTION("WorldShadow without BlendMode::Shadow")
    {
        RenderPassDescriptor d;
        d.pass = PassKind::WorldShadow;
        d.depth = DepthMode::Shadow;
        d.shader = ShaderFamily::Shadow;
        d.stencilExclusion = true;
        d.blend = BlendMode::Opaque; // <-- wrong
        REQUIRE(HasError(ValidateRenderPassDescriptor(d), "WorldShadow pass without BlendMode::Shadow"));
    }
    SECTION("WorldShadow without Shadow DepthMode")
    {
        RenderPassDescriptor d;
        d.pass = PassKind::WorldShadow;
        d.blend = BlendMode::Shadow;
        d.shader = ShaderFamily::Shadow;
        d.stencilExclusion = true;
        d.depth = DepthMode::Normal; // <-- wrong
        REQUIRE(HasError(ValidateRenderPassDescriptor(d), "WorldShadow pass without Shadow DepthMode"));
    }
    SECTION("WorldShadow without stencilExclusion")
    {
        RenderPassDescriptor d;
        d.pass = PassKind::WorldShadow;
        d.blend = BlendMode::Shadow;
        d.depth = DepthMode::Shadow;
        d.shader = ShaderFamily::Shadow;
        // stencilExclusion left at default false
        REQUIRE(HasError(ValidateRenderPassDescriptor(d), "WorldShadow pass without stencilExclusion"));
    }
}

TEST_CASE("ValidateRenderPassDescriptor: blend / alpha consistency", "[validate-descriptor][phase4]")
{
    SECTION("AlphaBlend without alpha info")
    {
        RenderPassDescriptor d;
        d.blend = BlendMode::AlphaBlend;
        d.alpha = AlphaMode::Disabled;
        REQUIRE(HasError(ValidateRenderPassDescriptor(d), "AlphaBlend with AlphaMode::Disabled"));
    }
    SECTION("AlphaBlend with Test is valid (e.g. cutout-with-blend)")
    {
        RenderPassDescriptor d;
        d.blend = BlendMode::AlphaBlend;
        d.alpha = AlphaMode::Test;
        d.alphaRef = 1;
        REQUIRE(IsRenderPassDescriptorValid(d));
    }
    SECTION("AlphaBlend with Blend is valid")
    {
        RenderPassDescriptor d;
        d.blend = BlendMode::AlphaBlend;
        d.alpha = AlphaMode::Blend;
        REQUIRE(IsRenderPassDescriptorValid(d));
    }
}

TEST_CASE("ValidateRenderPassDescriptor: AlphaFog requires AlphaBlend", "[validate-descriptor][phase4]")
{
    RenderPassDescriptor d;
    d.fog = FogMode::AlphaFog;
    d.alpha = AlphaMode::Test;
    d.blend = BlendMode::Opaque; // <-- wrong
    REQUIRE(HasError(ValidateRenderPassDescriptor(d), "FogMode::AlphaFog without BlendMode::AlphaBlend"));
}

TEST_CASE("ValidateRenderPassDescriptor: ShaderFamily / PassKind consistency", "[validate-descriptor][phase4]")
{
    SECTION("ShaderFamily::Shadow outside WorldShadow")
    {
        RenderPassDescriptor d;
        d.shader = ShaderFamily::Shadow;
        d.pass = PassKind::WorldOpaque; // <-- wrong
        REQUIRE(HasError(ValidateRenderPassDescriptor(d), "ShaderFamily::Shadow with non-WorldShadow PassKind"));
    }
    SECTION("ShaderFamily::Water outside WorldWater")
    {
        RenderPassDescriptor d;
        d.shader = ShaderFamily::Water;
        d.pass = PassKind::WorldOpaque; // <-- wrong
        REQUIRE(HasError(ValidateRenderPassDescriptor(d), "ShaderFamily::Water with non-WorldWater PassKind"));
    }
}

TEST_CASE("ValidateRenderPassDescriptor: light volume requires Additive blend", "[validate-descriptor][phase4]")
{
    RenderPassDescriptor d;
    d.pass = PassKind::WorldLight;
    d.blend = BlendMode::Opaque; // <-- wrong
    REQUIRE(HasError(ValidateRenderPassDescriptor(d), "WorldLight pass without BlendMode::Additive"));
}

TEST_CASE("ValidateRenderPassDescriptor: SurfaceOverlay requires OnSurface", "[validate-descriptor][phase4]")
{
    RenderPassDescriptor d;
    d.pass = PassKind::SurfaceOverlay;
    d.surface = SurfaceMode::Default; // <-- wrong
    REQUIRE(HasError(ValidateRenderPassDescriptor(d), "SurfaceOverlay pass without SurfaceMode::OnSurface"));
}

TEST_CASE("ValidateRenderPassDescriptor: ShadowDarkPolygon only on Shadow pass", "[validate-descriptor][phase4]")
{
    RenderPassDescriptor d;
    d.lighting = LightingMode::ShadowDarkPolygon;
    d.pass = PassKind::WorldOpaque; // <-- wrong
    REQUIRE(HasError(ValidateRenderPassDescriptor(d), "LightingMode::ShadowDarkPolygon outside WorldShadow pass"));
}

TEST_CASE("ValidateRenderPassDescriptor: ScreenSpace3D must have fog disabled", "[validate-descriptor][phase4]")
{
    RenderPassDescriptor d;
    d.pass = PassKind::ScreenSpace3D;
    d.fog = FogMode::Enabled; // <-- wrong: 3D UI draws after the world
    REQUIRE(HasError(ValidateRenderPassDescriptor(d), "ScreenSpace3D pass with FogMode::Enabled"));

    d.fog = FogMode::Disabled;
    REQUIRE(IsRenderPassDescriptorValid(d));
}

TEST_CASE("ValidateRenderPassDescriptor: OnSurface outside SurfaceOverlay/WorldShadow is invalid",
          "[validate-descriptor][phase4]")
{
    // The builder routes every OnSurface draw to SurfaceOverlay (shadow
    // excepted) — an OnSurface descriptor in another pass family means
    // a producer skipped the translation.
    RenderPassDescriptor d;
    d.surface = SurfaceMode::OnSurface;
    d.pass = PassKind::WorldOpaque; // <-- wrong
    REQUIRE(HasError(ValidateRenderPassDescriptor(d), "SurfaceMode::OnSurface outside SurfaceOverlay"));

    d.pass = PassKind::SurfaceOverlay;
    REQUIRE(IsRenderPassDescriptorValid(d));

    d.pass = PassKind::WorldShadow; // shadow keeps the surface flag; its own
    d.blend = BlendMode::Shadow;    // pass invariants still apply
    d.depth = DepthMode::Shadow;
    d.stencilExclusion = true;
    d.shader = ShaderFamily::Shadow;
    d.lighting = LightingMode::ShadowDarkPolygon;
    d.fog = FogMode::Disabled;
    d.alpha = AlphaMode::Test;
    REQUIRE(IsRenderPassDescriptorValid(d));
}
