#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Core/Types.hpp>
#include <Poseidon/Graphics/Rendering/Shape/ClipShape.hpp>
#include <Poseidon/Graphics/Rendering/Primitives/Poly.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/Graphics/Core/TLVertex.hpp>
#include <Poseidon/Graphics/Dummy/EngineDummy.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

// ShapeSection::PrepareTL must inherit a small set of routing bits from the
// caller spec (NoDropdown | FogDisabled | DisableSun | IsColored) into the
// section's effective spec before the section reaches the backend. Without
// inheritance, an object that decides at draw time "this is cockpit / fog-
// disabled / light_disc-disabled / constant-colored" loses that meaning at the
// section boundary and the backend binds world-default state.
//
// This test exercises the boundary directly by installing a recording engine
// that captures the specFlags passed to PrepareTriangleTL — the closest hop
// downstream of PolyProperties::PrepareTL. Reverting the inheritance change
// in ShapeSection::PrepareTL drops the caller bits from lastTriSpec and the
// per-bit REQUIREs fail.

using namespace Poseidon;
namespace
{

class RecordingEngine : public EngineDummy
{
  public:
    int lastTriSpec = -1;
    int lastMatSpec = -1;
    int triCallCount = 0;
    int matCallCount = 0;

    Poseidon::render::LegacySpec lastMatSpecTyped;
    Poseidon::render::LegacySpec lastTriSpecTyped;

    void SetMaterial(const TLMaterial& /*mat*/, const LightList& /*lights*/,
                     const Poseidon::render::LegacySpec& spec) override
    {
        lastMatSpec = static_cast<int>(Poseidon::render::MergeLegacy(spec));
        lastMatSpecTyped = spec;
        ++matCallCount;
    }

    void PrepareTriangleTL(const MipInfo& /*mip*/, const Poseidon::render::LegacySpec& spec) override
    {
        lastTriSpec = static_cast<int>(Poseidon::render::MergeLegacy(spec));
        lastTriSpecTyped = spec;
        ++triCallCount;
    }
};

struct GEngineGuard
{
    Engine* prev;
    explicit GEngineGuard(Engine* tmp) : prev(GEngine) { GEngine = tmp; }
    ~GEngineGuard() { GEngine = prev; }
};

// Default-initialize a ShapeSection with no surface material, no texture,
// and an explicit caller-supplied section.properties._special baseline.
void initBareSection(ShapeSection& s, int sectionSpec = 0)
{
    s.properties.Init();
    s.properties.SetSpecial(sectionSpec);
    s.material = 0;
    s.surfMat = nullptr;
}

} // namespace

TEST_CASE("ShapeSection::PrepareTL inherits routing bits from caller spec", "[Shape][inheritance]")
{
    RecordingEngine rec;
    GEngineGuard guard(&rec);

    TLMaterial mat;
    LightList lights;

    SECTION("NoDropdown reaches PrepareTriangleTL via section path")
    {
        ShapeSection section;
        initBareSection(section);
        section.PrepareTL(mat, lights, NoDropdown);
        REQUIRE(rec.triCallCount == 1);
        REQUIRE((rec.lastTriSpec & NoDropdown) != 0);
    }

    SECTION("FogDisabled reaches PrepareTriangleTL via section path")
    {
        ShapeSection section;
        initBareSection(section);
        section.PrepareTL(mat, lights, FogDisabled);
        REQUIRE(rec.triCallCount == 1);
        REQUIRE((rec.lastTriSpec & FogDisabled) != 0);
    }

    SECTION("DisableSun reaches PrepareTriangleTL via section path")
    {
        ShapeSection section;
        initBareSection(section);
        section.PrepareTL(mat, lights, DisableSun);
        REQUIRE(rec.triCallCount == 1);
        REQUIRE((rec.lastTriSpec & DisableSun) != 0);
    }

    SECTION("IsColored reaches PrepareTriangleTL via section path")
    {
        ShapeSection section;
        initBareSection(section);
        section.PrepareTL(mat, lights, IsColored);
        REQUIRE(rec.triCallCount == 1);
        REQUIRE((rec.lastTriSpec & IsColored) != 0);
    }

    SECTION("Combined cockpit-like spec carries through")
    {
        ShapeSection section;
        initBareSection(section);
        section.PrepareTL(mat, lights, NoDropdown | FogDisabled | DisableSun | IsColored);
        REQUIRE(rec.triCallCount == 1);
        REQUIRE((rec.lastTriSpec & NoDropdown) != 0);
        REQUIRE((rec.lastTriSpec & FogDisabled) != 0);
        REQUIRE((rec.lastTriSpec & DisableSun) != 0);
        REQUIRE((rec.lastTriSpec & IsColored) != 0);
    }

    SECTION("Typed LegacySpec arrives consistent with int specFlags (Phase 1.2)")
    {
        // The plumbing must guarantee the typed form passed to
        // PrepareTriangleTL exactly matches SplitLegacy(intSpec) — that
        // contract is what lets Phase 1.3 consumers read from the typed
        // form without re-deriving it.
        ShapeSection section;
        initBareSection(section, IsAlpha);
        section.PrepareTL(mat, lights, NoDropdown | FogDisabled | DisableSun);
        REQUIRE(rec.triCallCount == 1);
        REQUIRE(rec.lastTriSpecTyped == Poseidon::render::SplitLegacy(rec.lastTriSpec));
        // Spot-check category membership through the typed view.
        REQUIRE(Poseidon::render::Has(rec.lastTriSpecTyped.routing, Poseidon::render::Routing::NoDropdown));
        REQUIRE(Poseidon::render::Has(rec.lastTriSpecTyped.routing, Poseidon::render::Routing::FogDisabled));
        REQUIRE(Poseidon::render::Has(rec.lastTriSpecTyped.material, Poseidon::render::Material::DisableSun));
        REQUIRE(Poseidon::render::Has(rec.lastTriSpecTyped.backend, Poseidon::render::Backend::IsAlpha));
    }

    SECTION("Non-routing caller bits do not leak into section spec")
    {
        // IsAlpha and IsTransparent are backend pipeline facts owned by the
        // section itself; the caller spec should not be able to retro-set
        // them via PrepareTL inheritance.
        ShapeSection section;
        initBareSection(section);
        section.PrepareTL(mat, lights, IsAlpha | IsTransparent | NoZBuf);
        REQUIRE(rec.triCallCount == 1);
        REQUIRE((rec.lastTriSpec & IsAlpha) == 0);
        REQUIRE((rec.lastTriSpec & IsTransparent) == 0);
        REQUIRE((rec.lastTriSpec & NoZBuf) == 0);
    }

    SECTION("Section's own properties.Special() is preserved alongside inherited bits")
    {
        ShapeSection section;
        initBareSection(section, IsAlpha | IsTransparent);
        section.PrepareTL(mat, lights, NoDropdown | FogDisabled);
        REQUIRE(rec.triCallCount == 1);
        // Section-owned bits stay
        REQUIRE((rec.lastTriSpec & IsAlpha) != 0);
        REQUIRE((rec.lastTriSpec & IsTransparent) != 0);
        // Inherited bits arrive
        REQUIRE((rec.lastTriSpec & NoDropdown) != 0);
        REQUIRE((rec.lastTriSpec & FogDisabled) != 0);
    }

    SECTION("SetMaterial sees the full caller spec, independent of inheritance")
    {
        // SetMaterial is called with the unmasked caller spec — that path
        // already worked before the inheritance fix. Asserting it locks in
        // that the fix did not change the SetMaterial contract.
        ShapeSection section;
        initBareSection(section);
        section.PrepareTL(mat, lights, NoDropdown | FogDisabled | DisableSun | IsColored | IsAlpha);
        REQUIRE(rec.matCallCount == 1);
        REQUIRE((rec.lastMatSpec & NoDropdown) != 0);
        REQUIRE((rec.lastMatSpec & FogDisabled) != 0);
        REQUIRE((rec.lastMatSpec & DisableSun) != 0);
        REQUIRE((rec.lastMatSpec & IsColored) != 0);
        REQUIRE((rec.lastMatSpec & IsAlpha) != 0);
    }
}
