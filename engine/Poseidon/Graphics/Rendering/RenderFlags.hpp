#pragma once

#include <cstdint>
#include <type_traits>

// Strong-typed split of the `int spec` bitfield (see
// engine/Poseidon/Core/Types.hpp) into three semantic categories.
//
// Each bit in the int spec maps to exactly one of:
//
//   Routing  — scene-level pass selection / draw-order / visibility:
//              NoDropdown, FogDisabled, IsColored, IsAlphaOrdered,
//              NoShadow, ShadowDisabled, IsHidden, IsHiddenProxy,
//              OnSurface, IsOnSurface, NoTexMerger.
//
//   Material — per-mesh material / lighting state inputs:
//              DisableSun, BestMipmap, IsAnimated.
//
//   Backend  — backend pipeline-state inputs (sampler, blend, depth,
//              z-bias, shader family hints): NoZBuf, NoZWrite, IsShadow,
//              IsAlpha, IsTransparent, IsWater, IsLight, PointSampling,
//              NoClamp, ClampU, ClampV, IsAlphaFog, DetailTexture,
//              SpecularTexture, ZBiasStep/Mask, SpecLighting, GrassTexture.
//
// Bit values are identical to the int form so the SplitLegacy / MergeLegacy
// roundtrip is lossless and the typed fields can be plumbed alongside the int.
//
// Bitwise ops are opt-in via the `enable_bitmask_ops` trait below, so
// `Routing r = Backend::NoZBuf;` is a compile error — the boundaries
// the design doc wants to enforce are checked at the type level.

namespace Poseidon
{
namespace render
{

enum class Routing : std::uint32_t
{
    None = 0,
    OnSurface = 0x00000002,
    IsOnSurface = 0x00000004,
    NoShadow = 0x00000020,
    ShadowDisabled = 0x00000080,
    IsAlphaOrdered = 0x00020000,
    NoDropdown = 0x00040000,
    FogDisabled = 0x00100000,
    IsColored = 0x00200000,
    IsHidden = 0x00400000,
    IsHiddenProxy = 0x10000000,
    NoTexMerger = 0x20000000,
};

enum class Material : std::uint32_t
{
    None = 0,
    IsAnimated = 0x00010000,
    BestMipmap = 0x00800000,
    DisableSun = 0x80000000,
};

enum class Backend : std::uint32_t
{
    None = 0,
    GrassTexture = 0x00000001,
    NoZBuf = 0x00000008,
    NoZWrite = 0x00000010,
    IsShadow = 0x00000040,
    IsAlpha = 0x00000100,
    IsTransparent = 0x00000200,
    IsWater = 0x00000400,
    IsLight = 0x00000800,
    PointSampling = 0x00001000,
    NoClamp = 0x00002000,
    ClampU = 0x00004000,
    ClampV = 0x00008000,
    IsAlphaFog = 0x00080000,
    DetailTexture = 0x01000000,
    SpecularTexture = 0x02000000,
    ZBiasStep = 0x04000000,
    ZBiasMaskHi = 0x08000000,
    SpecLighting = 0x40000000,
};

// Bitmask sum of the three category masks must exactly equal 0xFFFFFFFF —
// every legacy bit has a home, nothing overlaps.  Static assertion below
// makes a misplaced bit a compile error.
constexpr std::uint32_t RoutingMask =
    static_cast<std::uint32_t>(Routing::OnSurface) | static_cast<std::uint32_t>(Routing::IsOnSurface) |
    static_cast<std::uint32_t>(Routing::NoShadow) | static_cast<std::uint32_t>(Routing::ShadowDisabled) |
    static_cast<std::uint32_t>(Routing::IsAlphaOrdered) | static_cast<std::uint32_t>(Routing::NoDropdown) |
    static_cast<std::uint32_t>(Routing::FogDisabled) | static_cast<std::uint32_t>(Routing::IsColored) |
    static_cast<std::uint32_t>(Routing::IsHidden) | static_cast<std::uint32_t>(Routing::IsHiddenProxy) |
    static_cast<std::uint32_t>(Routing::NoTexMerger);

constexpr std::uint32_t MaterialMask = static_cast<std::uint32_t>(Material::IsAnimated) |
                                       static_cast<std::uint32_t>(Material::BestMipmap) |
                                       static_cast<std::uint32_t>(Material::DisableSun);

constexpr std::uint32_t BackendMask =
    static_cast<std::uint32_t>(Backend::GrassTexture) | static_cast<std::uint32_t>(Backend::NoZBuf) |
    static_cast<std::uint32_t>(Backend::NoZWrite) | static_cast<std::uint32_t>(Backend::IsShadow) |
    static_cast<std::uint32_t>(Backend::IsAlpha) | static_cast<std::uint32_t>(Backend::IsTransparent) |
    static_cast<std::uint32_t>(Backend::IsWater) | static_cast<std::uint32_t>(Backend::IsLight) |
    static_cast<std::uint32_t>(Backend::PointSampling) | static_cast<std::uint32_t>(Backend::NoClamp) |
    static_cast<std::uint32_t>(Backend::ClampU) | static_cast<std::uint32_t>(Backend::ClampV) |
    static_cast<std::uint32_t>(Backend::IsAlphaFog) | static_cast<std::uint32_t>(Backend::DetailTexture) |
    static_cast<std::uint32_t>(Backend::SpecularTexture) | static_cast<std::uint32_t>(Backend::ZBiasStep) |
    static_cast<std::uint32_t>(Backend::ZBiasMaskHi) | static_cast<std::uint32_t>(Backend::SpecLighting);

static_assert((RoutingMask & MaterialMask) == 0, "Routing/Material bit overlap");
static_assert((RoutingMask & BackendMask) == 0, "Routing/Backend bit overlap");
static_assert((MaterialMask & BackendMask) == 0, "Material/Backend bit overlap");
static_assert((RoutingMask | MaterialMask | BackendMask) == 0xFFFFFFFFu, "Spec bit coverage incomplete");

// Bitmask operator opt-in.  Specialize for the three enum class types so
// the compiler accepts `Routing::NoDropdown | Routing::FogDisabled` but
// rejects `Routing::NoDropdown | Backend::NoZBuf`.
template <typename E>
struct enable_bitmask_ops : std::false_type
{
};

template <>
struct enable_bitmask_ops<Routing> : std::true_type
{
};
template <>
struct enable_bitmask_ops<Material> : std::true_type
{
};
template <>
struct enable_bitmask_ops<Backend> : std::true_type
{
};

template <typename E>
constexpr std::enable_if_t<enable_bitmask_ops<E>::value, E> operator|(E a, E b)
{
    return static_cast<E>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

template <typename E>
constexpr std::enable_if_t<enable_bitmask_ops<E>::value, E> operator&(E a, E b)
{
    return static_cast<E>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}

template <typename E>
constexpr std::enable_if_t<enable_bitmask_ops<E>::value, E> operator^(E a, E b)
{
    return static_cast<E>(static_cast<std::uint32_t>(a) ^ static_cast<std::uint32_t>(b));
}

template <typename E>
constexpr std::enable_if_t<enable_bitmask_ops<E>::value, E> operator~(E a)
{
    return static_cast<E>(~static_cast<std::uint32_t>(a));
}

template <typename E>
constexpr std::enable_if_t<enable_bitmask_ops<E>::value, E&> operator|=(E& a, E b)
{
    a = a | b;
    return a;
}

template <typename E>
constexpr std::enable_if_t<enable_bitmask_ops<E>::value, E&> operator&=(E& a, E b)
{
    a = a & b;
    return a;
}

template <typename E>
constexpr std::enable_if_t<enable_bitmask_ops<E>::value, bool> Has(E a, E bit)
{
    return (static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(bit)) != 0;
}

// Triple-category container carrying the typed Routing / Material / Backend
// fields together, plumbed alongside the `int spec` form.
struct LegacySpec
{
    Routing routing = Routing::None;
    Material material = Material::None;
    Backend backend = Backend::None;

    bool operator==(const LegacySpec& rhs) const
    {
        return routing == rhs.routing && material == rhs.material && backend == rhs.backend;
    }
    bool operator!=(const LegacySpec& rhs) const { return !(*this == rhs); }
};

// Lossless split of a legacy int spec into the three categories.
constexpr LegacySpec SplitLegacy(std::uint32_t spec)
{
    LegacySpec out;
    out.routing = static_cast<Routing>(spec & RoutingMask);
    out.material = static_cast<Material>(spec & MaterialMask);
    out.backend = static_cast<Backend>(spec & BackendMask);
    return out;
}

constexpr LegacySpec SplitLegacy(int spec)
{
    return SplitLegacy(static_cast<std::uint32_t>(spec));
}

// Lossless merge back into an int.
constexpr std::uint32_t MergeLegacy(const LegacySpec& s)
{
    return static_cast<std::uint32_t>(s.routing) | static_cast<std::uint32_t>(s.material) |
           static_cast<std::uint32_t>(s.backend);
}

// True when the routing belongs to the surface-overlay group (roads, tyre
// tracks, marks, craters) — geometry drawn as a polygon-offset decal over
// the terrain.
constexpr bool IsOnSurfaceRouting(Routing r)
{
    return Has(r, Routing::OnSurface) || Has(r, Routing::IsOnSurface);
}

constexpr bool IsOnSurfaceSpec(std::uint32_t spec)
{
    return IsOnSurfaceRouting(SplitLegacy(spec).routing);
}

constexpr bool IsOnSurfaceSpec(int spec)
{
    return IsOnSurfaceSpec(static_cast<std::uint32_t>(spec));
}

} // namespace render

} // namespace Poseidon
