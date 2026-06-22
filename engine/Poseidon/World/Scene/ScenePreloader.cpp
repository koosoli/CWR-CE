#include <Poseidon/World/Scene/ScenePreloader.hpp>

#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/Scene/Object.hpp>
#include <Poseidon/World/Scene/ObjLine.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Foundation/Logging/Logging.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>

#include <cstdlib>
#include <stdio.h>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

extern ParamFile Remaster;

namespace Poseidon
{

static void AdaptVolumeLight(LODShape* lodShape)
{
    lodShape->OrSpecial(NoShadow | IsAlpha | IsLight | NoZWrite | ClampU | ClampV | IsAlphaFog | IsColored);
}

namespace
{
struct SlotDef
{
    const char* className;
    PreloadedShape slot;
    bool reversed;
    bool animated;
};

const SlotDef kSimpleSlots[] = {
    {"CraterShell", CraterShell, false, false},
    {"SlopBlood", SlopBlood, false, false},
    {"CloudletBasic", CloudletBasic, false, false},
    {"CloudletFire", CloudletFire, false, false},
    {"CloudletWater", CloudletWater, false, false},
    {"CinemaBorder", CinemaBorder, false, false},
    {"CobraLight", CobraLight, false, false},
    {"SphereLight", SphereLight, false, false},
    {"HalfLight", HalfLight, false, false},
    {"Marker", Marker, false, false},
    {"FootStepL", FootStepL, false, false},
    {"FootStepR", FootStepR, false, false},
    {"ForceArrowModel", ForceArrowModel, false, false},
    {"SphereModel", SphereModel, false, false},
    {"RectangleModel", RectangleModel, false, false},
};

Ref<LODShapeWithShadow> LoadShape(const ParamEntry& cfgScenePreload, const char* className, bool reversed,
                                  bool animated)
{
    RStringB name = cfgScenePreload >> className >> "model";
    if (name.GetLength() == 0)
        return nullptr;
    Ref<LODShapeWithShadow> shape = Shapes.New(name, reversed, animated);
    if (!shape)
    {
        LOG_ERROR(Graphics, "ScenePreloader: CfgScenePreload.{}.model='{}' failed to load", className,
                  static_cast<const char*>(name));
        if (Foundation::LoggingSystem::IsStrictMode())
            std::exit(1);
        // --no-strict (the build players run): a missing preload model is logged but
        // not fatal — return a null shape and let the engine boot rather than exit(1).
    }
    return shape;
}
} // namespace

ScenePreloader& ScenePreloader::Instance()
{
    static ScenePreloader instance;
    return instance;
}

std::vector<std::string> ScenePreloader::SlotClassNames()
{
    std::vector<std::string> names;
    for (const auto& def : kSimpleSlots)
        names.emplace_back(def.className);
    names.emplace_back("Cloud1");
    names.emplace_back("Cloud2");
    names.emplace_back("Cloud3");
    names.emplace_back("Cloud4");
    names.emplace_back("CollisionStar");
    return names;
}

void ScenePreloader::Initialize(Scene& scene)
{
    if (_initialized)
        return;

    const ParamEntry& sp = Remaster >> "CfgScenePreload";

    for (const auto& def : kSimpleSlots)
        scene.SetPreloaded(def.slot, LoadShape(sp, def.className, def.reversed, def.animated));

    // Cloud billboards — 4 rotated layers with per-vertex clip flags and
    // animation enabled.
    for (int c = 0; c < 4; c++)
    {
        char cls[32];
        snprintf(cls, sizeof(cls), "Cloud%d", c + 1);
        PreloadedShape cs = static_cast<PreloadedShape>(Cloud1 + c);
        Ref<LODShapeWithShadow> shape = LoadShape(sp, cls, false, false);
        scene.SetPreloaded(cs, shape);
        if (!shape)
            continue;
        shape->OrSpecial(NoZBuf | IsAlpha | IsAlphaFog);
        Shape* shape0 = shape->LevelOpaque(0);
        for (int v = 0; v < shape0->NPos(); v++)
            shape0->SetClip(v, ClipFogSky | ClipLightCloud | (ClipAll & ~ClipBack));
        shape0->CalculateHints();
        shape->CalculateHints();
        shape->AllowAnimation();
    }

    // Pipeline flags by slot.
    const int cloudletSpec = ClampU | ClampV | NoZWrite | IsAlpha | IsAlphaFog | NoShadow | IsColored;
    if (auto* s = scene.Preloaded(CloudletBasic))
        s->OrSpecial(cloudletSpec);
    if (auto* s = scene.Preloaded(CloudletFire))
        s->OrSpecial(cloudletSpec);
    if (auto* s = scene.Preloaded(CloudletWater))
        s->OrSpecial(cloudletSpec);

    const int craterSpec = ClampU | ClampV | NoZWrite | IsAlpha | IsAlphaFog | OnSurface | NoShadow | IsColored;
    if (auto* s = scene.Preloaded(CraterShell))
        s->OrSpecial(craterSpec);
    if (auto* s = scene.Preloaded(SlopBlood))
        s->OrSpecial(craterSpec);

    if (auto* s = scene.Preloaded(ForceArrowModel))
        s->OrSpecial(IsColored | IsAlpha | IsAlphaFog);
    if (auto* s = scene.Preloaded(SphereModel))
        s->OrSpecial(IsColored | IsAlpha | IsAlphaFog);
    if (auto* s = scene.Preloaded(RectangleModel))
        s->OrSpecial(IsColored | IsAlpha | IsAlphaFog);

    const int footStepFlags = IsAlpha | NoShadow | NoZWrite | IsAlphaFog | OnSurface | IsColored;
    if (auto* s = scene.Preloaded(FootStepL))
        s->OrSpecial(footStepFlags);
    if (auto* s = scene.Preloaded(FootStepR))
        s->OrSpecial(footStepFlags);

    // Procedural shapes — generated, not loaded.  Belong here logically
    // alongside the rest of the scene-wide preload set.
    scene.SetPreloaded(BulletLine, ObjectLine::CreateShape());

    if (auto* s = scene.Preloaded(CobraLight))
        AdaptVolumeLight(s);
    if (auto* s = scene.Preloaded(SphereLight))
        AdaptVolumeLight(s);
    if (auto* s = scene.Preloaded(HalfLight))
        AdaptVolumeLight(s);
    if (auto* s = scene.Preloaded(Marker))
        AdaptVolumeLight(s);

    // CollisionStar uses a dedicated field (not in _preloaded[]).
    {
        RStringB colName = sp >> "CollisionStar" >> "model";
        if (colName.GetLength() > 0)
        {
            LODShapeWithShadow* collisionShape = Shapes.New(colName, false, false);
            if (!collisionShape)
            {
                LOG_ERROR(Graphics, "ScenePreloader: CfgScenePreload.CollisionStar.model='{}' failed to load",
                          static_cast<const char*>(colName));
                std::exit(1);
            }
            collisionShape->OrSpecial(IsAlpha | IsAlphaFog | IsColored);
            scene.SetCollisionStar(new ObjectColored(collisionShape, -1));
        }
    }

    _initialized = true;
    LOG_INFO(Graphics, "ScenePreloader initialized ({} slot classes)", SlotClassNames().size());
}

void ScenePreloader::Shutdown()
{
    _initialized = false;
}

} // namespace Poseidon
