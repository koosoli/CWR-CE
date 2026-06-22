#pragma once

#include <Poseidon/Graphics/Rendering/RenderPassDescriptor.hpp>

// Descriptor validation.
//
// `ValidateRenderPassDescriptor` returns `nullptr` if the descriptor is
// internally consistent, or a static error string describing the first
// invariant violation found.  Use in debug paths:
//
//   const char* err = render::ValidateRenderPassDescriptor(d);
//   PoseidonAssertMsg(!err, err);
//
// or as a non-fatal log line:
//
//   if (const char* err = render::ValidateRenderPassDescriptor(d))
//       LOG_DEBUG(Graphics, "descriptor invariant: {}", err);
//
// The checked invariants are combinations that signal a producer /
// translation bug, not a runtime data error.  Returns nullptr for the
// common case so the hot path costs almost nothing in optimized builds.

namespace Poseidon
{
namespace render
{

inline const char* ValidateRenderPassDescriptor(const RenderPassDescriptor& d)
{
    // 1. Shader family / pass-kind consistency
    if (d.shader == ShaderFamily::Shadow && d.pass != PassKind::WorldShadow)
        return "ShaderFamily::Shadow with non-WorldShadow PassKind";
    if (d.shader == ShaderFamily::Water && d.pass != PassKind::WorldWater)
        return "ShaderFamily::Water with non-WorldWater PassKind";

    // 2. Shadow-pass invariants
    if (d.pass == PassKind::WorldShadow)
    {
        if (d.blend != BlendMode::Shadow)
            return "WorldShadow pass without BlendMode::Shadow";
        if (d.depth != DepthMode::Shadow)
            return "WorldShadow pass without Shadow DepthMode";
        if (!d.stencilExclusion)
            return "WorldShadow pass without stencilExclusion";
        if (d.shader != ShaderFamily::Shadow)
            return "WorldShadow pass without ShaderFamily::Shadow";
    }

    // 4. Cockpit-pass invariant
    // Cockpit-family draws are routed late (post-world) so they should
    // not pick up world fog.  A producer that hands the descriptor
    // CockpitOpaque with FogMode::Enabled has lost the fog-disable
    // signal somewhere upstream.
    const bool isCockpit = (d.pass == PassKind::CockpitOpaque || d.pass == PassKind::CockpitCutout ||
                            d.pass == PassKind::CockpitTransparent);
    if (isCockpit && d.fog == FogMode::Enabled)
        return "Cockpit pass with FogMode::Enabled (cockpit/first-person should have fog disabled)";

    // ScreenSpace3D (3D UI) draws after the world for the same reason —
    // world fog on a UI element means the fog-disable signal was lost.
    if (d.pass == PassKind::ScreenSpace3D && d.fog == FogMode::Enabled)
        return "ScreenSpace3D pass with FogMode::Enabled (3D UI should have fog disabled)";

    // 5. Surface-overlay must be OnSurface (and vice versa)
    if (d.pass == PassKind::SurfaceOverlay && d.surface != SurfaceMode::OnSurface)
        return "SurfaceOverlay pass without SurfaceMode::OnSurface";
    // The builder routes every OnSurface draw to SurfaceOverlay (shadow
    // excepted) — an OnSurface descriptor in another pass family skipped
    // the translation.
    if (d.surface == SurfaceMode::OnSurface && d.pass != PassKind::SurfaceOverlay && d.pass != PassKind::WorldShadow)
        return "SurfaceMode::OnSurface outside SurfaceOverlay/WorldShadow pass";

    // 6. Alpha / blend consistency
    // A draw flagged transparent (alpha-blended) without any alpha
    // information — neither test ref nor blend — would write the
    // entire vertex color to the framebuffer regardless of alpha,
    // which is almost never the producer's intent.
    if (d.blend == BlendMode::AlphaBlend && d.alpha == AlphaMode::Disabled)
        return "BlendMode::AlphaBlend with AlphaMode::Disabled (transparent draw with no alpha info)";

    // 7. AlphaFog fog mode requires alpha-blend
    // FogMode::AlphaFog encodes the `IsAlphaFog` path: alpha attenuation
    // replaces depth fog.  The blend mode must agree.
    if (d.fog == FogMode::AlphaFog && d.blend != BlendMode::AlphaBlend)
        return "FogMode::AlphaFog without BlendMode::AlphaBlend";

    // 8. Light-volume invariant
    if (d.pass == PassKind::WorldLight && d.blend != BlendMode::Additive)
        return "WorldLight pass without BlendMode::Additive";

    // 9. ShadowDarkPolygon only on Shadow pass
    if (d.lighting == LightingMode::ShadowDarkPolygon && d.pass != PassKind::WorldShadow)
        return "LightingMode::ShadowDarkPolygon outside WorldShadow pass";

    // 10. Cull / front-face: degenerate combinations
    // (No invariants here yet — CullMode::None + any FrontFaceMode is
    //  valid for double-sided draws; descriptor doesn't restrict it.)

    return nullptr; // valid
}

inline bool IsRenderPassDescriptorValid(const RenderPassDescriptor& d)
{
    return ValidateRenderPassDescriptor(d) == nullptr;
}

} // namespace render

} // namespace Poseidon
