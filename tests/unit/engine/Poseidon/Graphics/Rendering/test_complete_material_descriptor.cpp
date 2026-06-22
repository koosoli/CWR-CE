#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Graphics/Rendering/RenderPassDescriptor.hpp>
#include <Poseidon/Graphics/Rendering/BuildRenderPassDescriptor.hpp>
#include <Poseidon/Graphics/Rendering/ValidateRenderPassDescriptor.hpp>

#include <type_traits>
#include <catch2/catch_message.hpp>

using namespace Poseidon::render;

// I-26 / B-032: Every draw specifies a complete material state - no
// uniform inherits leftover values from a prior draw.
//
// B-032 was the RHI-layer `DrawLine` not resetting `uHasTexture` so
// the next draw sampled the previous texture into the line.  The
// shipping GL33 backend doesn't have a `uHasTexture` uniform at all;
// `ShaderFamily` carries the texture-sampling routing.  `Flat`
// samples nothing.  `Normal` / `Detail` / `Grass` each sample
// specific texture slots.  Choosing a shader family IS the binding
// - a draw can't be "partially textured" because the descriptor's
// shader field is one byte that picks the whole sampling regime at
// once.
//
// These tests pin the structural guarantees:
//   - the descriptor has a defaulted `shader` field (no "Inherit"
//     sentinel value),
//   - the field has a fully-defined enum class type (no integer
//     promotion lets a producer pass an out-of-range value),
//   - `BuildRenderPassDescriptor` produces a descriptor whose
//     `shader` field is one of the defined enum values for every
//     reasonable input,
//   - `ValidateRenderPassDescriptor` rejects descriptors with
//     malformed material/shader combinations.

TEST_CASE("I-26: RenderPassDescriptor.shader has a sensible default", "[Rendering][CompleteMaterial][I-26]")
{
    RenderPassDescriptor d{};
    REQUIRE(d.shader == ShaderFamily::Normal);
}

TEST_CASE("I-26: every RenderPassDescriptor field is defaulted to a non-sentinel value",
          "[Rendering][CompleteMaterial][I-26]")
{
    // A default-constructed descriptor must be VALID - i.e. running
    // through ValidateRenderPassDescriptor returns nullptr.  If a
    // future field gets a default that requires explicit override,
    // the validator catches the inheritance gap before any draw lands.
    RenderPassDescriptor d{};
    REQUIRE(ValidateRenderPassDescriptor(d) == nullptr);
}

TEST_CASE("I-26: ShaderFamily is an enum class - no implicit conversion from int",
          "[Rendering][CompleteMaterial][I-26]")
{
    static_assert(std::is_enum_v<ShaderFamily>, "ShaderFamily must be an enum");
    static_assert(!std::is_convertible_v<int, ShaderFamily>, "int must not implicitly convert to ShaderFamily - "
                                                             "producers must pick a named value at the call site");
    static_assert(!std::is_convertible_v<ShaderFamily, int>, "ShaderFamily must not silently leak as int");
    SUCCEED("Type system blocks 'partial material' via scoped enum.");
}

TEST_CASE("I-26: BuildRenderPassDescriptor produces a defined shader family for every routing/material/backend combo",
          "[Rendering][CompleteMaterial][I-26]")
{
    // Walk a representative set of bit combinations through
    // BuildRenderPassDescriptor; assert the resulting descriptor's
    // `shader` is one of the named ShaderFamily values.  An
    // uninitialised / sentinel shader value would let the bound
    // program inherit from the previous draw - exactly the B-032
    // leak class.
    BuildContext ctx;
    ctx.isIn3DPass = true;

    auto isNamedShader = [](ShaderFamily s)
    {
        switch (s)
        {
            case ShaderFamily::Normal:
            case ShaderFamily::Shadow:
            case ShaderFamily::Water:
            case ShaderFamily::Detail:
            case ShaderFamily::Grass:
            case ShaderFamily::Flat:
                return true;
        }
        return false;
    };

    // Sabotage: a future change that lets BuildRenderPassDescriptor emit
    // an unnamed shader value would let `d.shader` cast back into an
    // enum value isNamedShader doesn't recognise.  We can't sabotage
    // BuildRenderPassDescriptor easily here, so verify the negative
    // check inline instead:
    REQUIRE_FALSE(isNamedShader(static_cast<ShaderFamily>(0xFF)));

    LegacySpec cases[] = {
        // Pure opaque world geometry.
        LegacySpec{},
        // Shadow (B-010 sibling - must route to Shadow family).
        []
        {
            LegacySpec s;
            s.backend = Backend::IsShadow;
            return s;
        }(),
        // Water (must route to Water family).
        []
        {
            LegacySpec s;
            s.backend = Backend::IsWater;
            return s;
        }(),
        // Alpha-blended transparent.
        []
        {
            LegacySpec s;
            s.backend = Backend::IsAlpha | Backend::IsTransparent;
            return s;
        }(),
        // Detail multitexture.
        []
        {
            LegacySpec s;
            s.backend = Backend::DetailTexture;
            return s;
        }(),
        // Grass multitexture.
        []
        {
            LegacySpec s;
            s.backend = Backend::GrassTexture;
            return s;
        }(),
        // Surface decal.
        []
        {
            LegacySpec s;
            s.routing = Routing::OnSurface;
            return s;
        }(),
    };

    BuildContext multitexCtx = ctx;
    multitexCtx.isMultitexturing = true;
    for (const auto& s : cases)
    {
        const auto d = BuildRenderPassDescriptor(s, multitexCtx);
        CAPTURE(static_cast<int>(d.shader));
        REQUIRE(isNamedShader(d.shader));
    }
}

TEST_CASE("I-26: texGen mode is paired with the shader family that consumes it", "[Rendering][CompleteMaterial][I-26]")
{
    // The descriptor's `texGen` field decides how UVs are generated;
    // a shader family that doesn't use generated UVs must keep
    // texGen at `None` (otherwise a stale Water / Grass texgen would
    // bleed into a Normal draw).
    BuildContext ctx;
    ctx.isIn3DPass = true;

    {
        LegacySpec s;
        s.backend = Backend::IsWater;
        const auto d = BuildRenderPassDescriptor(s, ctx);
        REQUIRE(d.shader == ShaderFamily::Water);
        REQUIRE(d.texGen == TexGenMode::Water);
    }
    {
        LegacySpec s; // default opaque
        const auto d = BuildRenderPassDescriptor(s, ctx);
        REQUIRE(d.shader == ShaderFamily::Normal);
        REQUIRE(d.texGen == TexGenMode::None);
    }
}

TEST_CASE("I-26: shader / pass consistency catches partial-material mismatches", "[Rendering][CompleteMaterial][I-26]")
{
    // `ValidateRenderPassDescriptor` already enforces a handful of
    // shader/pass coherence rules.  Spot-check the two most direct
    // I-26-relevant ones so a future relaxation breaks the test.
    {
        RenderPassDescriptor d;
        d.shader = ShaderFamily::Shadow;
        d.pass = PassKind::WorldOpaque; // mismatch: Shadow shader, non-Shadow pass
        REQUIRE(ValidateRenderPassDescriptor(d) != nullptr);
    }
    {
        RenderPassDescriptor d;
        d.shader = ShaderFamily::Water;
        d.pass = PassKind::WorldOpaque; // mismatch
        REQUIRE(ValidateRenderPassDescriptor(d) != nullptr);
    }
}
