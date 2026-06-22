#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Graphics/Rendering/RenderFlags.hpp>
#include <Poseidon/Graphics/Rendering/BuildRenderPassDescriptor.hpp>

#include <cstdint>
#include <type_traits>
#include <catch2/catch_message.hpp>

using namespace Poseidon::render;

// I-08 / B-010: One spec bit, one semantic meaning.
//
// B-010 was a chrome material rendering wrong after a refactor
// routed all `IsShadow` draws through `VSShadow`.  Some chrome
// assets had repurposed the `IsShadow` bit, and the refactor
// assumed every `IsShadow` meant stencil shadow.  The Phase 1
// type-level split (`Routing` / `Material` / `Backend`) makes the
// chrome-vs-shadow path unambiguous at the bit level.
//
// These tests pin the structural guarantees the split provides:
//   - bit coverage is disjoint at compile time (already
//     `static_assert`ed in RenderFlags.hpp; tests re-state the
//     property so test failure is the obvious signal),
//   - the three category masks don't share any bit,
//   - chrome material routing (SpecLighting / SpecularTexture)
//     never produces WorldShadow pass on its own,
//   - `IsShadow` routing produces WorldShadow + Shader::Shadow
//     deterministically and overrides chrome bits when both are
//     set (documents the precedence so a future refactor can't
//     silently flip the order).
//   - The type system rejects cross-category bitwise ops at
//     compile time.

TEST_CASE("I-08: Routing / Material / Backend masks are mutually disjoint", "[Rendering][SpecSplit][I-08]")
{
    REQUIRE((RoutingMask & MaterialMask) == 0u);
    REQUIRE((RoutingMask & BackendMask) == 0u);
    REQUIRE((MaterialMask & BackendMask) == 0u);
}

TEST_CASE("I-08: every legacy bit has a typed home", "[Rendering][SpecSplit][I-08]")
{
    // Total coverage must include every bit of the legacy uint32
    // spec - no bit is unclaimed.  If a new bit gets added to the
    // legacy stream without a typed home, SplitLegacy would silently
    // drop it.
    REQUIRE((RoutingMask | MaterialMask | BackendMask) == 0xFFFFFFFFu);
}

TEST_CASE("I-08: type system rejects cross-category bitwise OR", "[Rendering][SpecSplit][I-08]")
{
    // Same-category OR is legal - opt-in via `enable_bitmask_ops`.
    const Routing r = Routing::NoDropdown | Routing::FogDisabled;
    REQUIRE(Has(r, Routing::NoDropdown));
    REQUIRE(Has(r, Routing::FogDisabled));

    // Cross-category OR - `Routing::NoDropdown | Backend::NoZBuf` -
    // doesn't compile.  We can't write that line and assert on it
    // here without breaking the build, so the SFINAE trait is what
    // pins the invariant.  Spot-check both directions of the
    // template trait so a future "specialize for std::is_enum" change
    // can't quietly opt every enum in.
    static_assert(enable_bitmask_ops<Routing>::value, "Routing must have bitmask ops");
    static_assert(enable_bitmask_ops<Material>::value, "Material must have bitmask ops");
    static_assert(enable_bitmask_ops<Backend>::value, "Backend must have bitmask ops");
    static_assert(!enable_bitmask_ops<int>::value, "raw int must not");
    static_assert(!enable_bitmask_ops<std::uint32_t>::value, "uint32_t must not");
    SUCCEED("Cross-category OR is a compile error (see template traits above).");
}

TEST_CASE("I-08: SplitLegacy partitions every bit into exactly one category", "[Rendering][SpecSplit][I-08]")
{
    // For every single bit set in a uint32, SplitLegacy must put it
    // in exactly one of routing / material / backend (or none if it's
    // unclaimed - that's caught by the coverage assert above).
    for (int i = 0; i < 32; ++i)
    {
        const std::uint32_t bit = 1u << i;
        const LegacySpec s = SplitLegacy(bit);

        const std::uint32_t inRouting = static_cast<std::uint32_t>(s.routing);
        const std::uint32_t inMaterial = static_cast<std::uint32_t>(s.material);
        const std::uint32_t inBackend = static_cast<std::uint32_t>(s.backend);

        const int homes = (inRouting != 0 ? 1 : 0) + (inMaterial != 0 ? 1 : 0) + (inBackend != 0 ? 1 : 0);
        CAPTURE(i);
        CAPTURE(bit);
        REQUIRE(homes == 1); // exactly one category claims this bit
    }
}

TEST_CASE("I-08: MergeLegacy roundtrips through SplitLegacy losslessly", "[Rendering][SpecSplit][I-08]")
{
    // Sanity: a few realistic bit combinations survive split → merge.
    const std::uint32_t cases[] = {
        0u,
        0xFFFFFFFFu,
        static_cast<std::uint32_t>(Backend::IsShadow) | static_cast<std::uint32_t>(Backend::NoZWrite),
        static_cast<std::uint32_t>(Backend::SpecularTexture) | static_cast<std::uint32_t>(Backend::SpecLighting),
        static_cast<std::uint32_t>(Routing::OnSurface) | static_cast<std::uint32_t>(Routing::FogDisabled),
    };
    for (auto c : cases)
    {
        CAPTURE(c);
        REQUIRE(MergeLegacy(SplitLegacy(c)) == c);
    }
}

// ---------------------------------------------------------------------------
// Behavioural: chrome / shadow routing is unambiguous in the descriptor.
// ---------------------------------------------------------------------------

namespace
{
RenderPassDescriptor BuildFromBackend(Backend b, BuildContext ctx = {})
{
    LegacySpec s;
    s.backend = b;
    return BuildRenderPassDescriptor(s, ctx);
}
} // namespace

TEST_CASE("I-08: chrome material (SpecLighting + SpecularTexture) is not WorldShadow", "[Rendering][SpecSplit][I-08]")
{
    BuildContext ctx;
    ctx.isIn3DPass = true;
    ctx.isMultitexturing = true;
    const auto d = BuildFromBackend(Backend::SpecLighting | Backend::SpecularTexture, ctx);

    REQUIRE(d.pass != PassKind::WorldShadow);
    REQUIRE(d.shader != ShaderFamily::Shadow);
    REQUIRE(d.blend != BlendMode::Shadow);
    REQUIRE_FALSE(d.stencilExclusion);
}

TEST_CASE("I-08: IsShadow routes to WorldShadow + ShaderFamily::Shadow (B-010)", "[Rendering][SpecSplit][I-08]")
{
    BuildContext ctx;
    ctx.isIn3DPass = true;
    const auto d = BuildFromBackend(Backend::IsShadow, ctx);

    REQUIRE(d.pass == PassKind::WorldShadow);
    REQUIRE(d.shader == ShaderFamily::Shadow);
    REQUIRE(d.blend == BlendMode::Shadow);
    REQUIRE(d.stencilExclusion);
}

TEST_CASE("I-08: IsShadow + SpecLighting is documented precedence - shadow wins", "[Rendering][SpecSplit][I-08]")
{
    // If a future asset (or a future refactor that reintroduces
    // ambiguous bit reuse) lights up both IsShadow and a chrome
    // backend bit, the build descriptor picks the shadow branch
    // first.  Document the precedence so a swap of the if-else
    // order in BuildRenderPassDescriptor.hpp trips this test.
    BuildContext ctx;
    ctx.isIn3DPass = true;
    const auto d = BuildFromBackend(Backend::IsShadow | Backend::SpecLighting, ctx);

    REQUIRE(d.pass == PassKind::WorldShadow);
    REQUIRE(d.shader == ShaderFamily::Shadow);
}
