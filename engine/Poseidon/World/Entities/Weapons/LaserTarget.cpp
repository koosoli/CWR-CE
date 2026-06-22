#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Core/Config/UserConfig.hpp>
#include <Poseidon/World/Entities/Weapons/LaserTarget.hpp>
#include <Poseidon/AI/AI.hpp>
#include <Poseidon/Network/Network.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/Terrain/Landscape.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Dev/Diag/DiagModes.hpp>
#include <Poseidon/Foundation/Containers/StaticArray.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Math/MathOpt.hpp>
#include <Poseidon/Foundation/Types/LLinks.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

namespace Poseidon
{
LaserTargetType::LaserTargetType(const ParamEntry* param) : base(param)
{
    _scopeLevel = 1;
}

void LaserTargetType::Load(const ParamEntry& par)
{
    base::Load(par);
}
void LaserTargetType::InitShape()
{
    _scopeLevel = 2;
    base::InitShape();
    _shape->OrSpecial(IsColored | ZBiasMask | IsAlpha | IsAlphaFog);
    if (_shape->NLevels() >= 2)
    {
        Shape* points = _shape->Level(1);
        for (int i = 0; i < points->NVertex(); i++)
        {
            ClipFlags clip = points->Clip(i);
            clip &= ~ClipLightMask;
            clip |= ClipLightLine;
            points->SetClip(i, clip);
        }
        points->CalculateHints();
        _shape->CalculateHints();
    }
}

DEFINE_CASTING(LaserTarget)

LaserTarget::LaserTarget(EntityAIType* name, bool fullCreate) : base(name, fullCreate)
{
    static PackedColor laserCol(Color(1, 0, 0, 1));
    SetConstantColor(laserCol);
    _lastActivation = Glob.time;
}

void LaserTarget::CheckDesignatedTarget(Matrix4Par pos)
{
#if _ENABLE_CHEATS
    Object* odt = _designatedTarget;
#endif
    AUTO_STATIC_ARRAY(OLink<Object>, objects, 16);
    _designatedTarget = nullptr;
    // check if we are inside or very close of some object
    // if yes, it is designated target
    GLandscape->IsInside(objects, this, pos.Position(), ObjIntersectFire);
    if (objects.Size() > 0)
    {
        for (int i = 0; i < objects.Size(); i++)
        {
            if (objects[0]->GetShape())
            {
                _designatedTarget = objects[0];
                PoseidonAssert(!dynamic_cast<LaserTarget*>(_designatedTarget.GetLink()));
                break;
            }
        }
    }
    Object* obj = GLandscape->NearestObject(pos.Position(), 50, (ObjectType)(Primary | TypeVehicle), this);
    if (obj)
    {
        LODShape* shape = obj->GetShape();
        if (shape)
        {
            Vector3Val geomCenter = obj->PositionModelToWorld(shape->GeometryCenter());
            float objDist2 = geomCenter.Distance2(pos.Position());
            if (objDist2 >= Square(shape->GeometrySphere()))
            {
                // if no normal object is near, try road objects
                obj = nullptr;
            }
            _designatedTarget = obj;
            PoseidonAssert(!dynamic_cast<LaserTarget*>(_designatedTarget.GetLink()));
        }
    }
    if (!obj)
    {
        obj = GLandscape->NearestObject(pos.Position(), 50, Network, this);
        if (obj)
        {
            LODShape* shape = obj->GetShape();
            if (shape)
            {
                Vector3Val geomCenter = obj->PositionModelToWorld(shape->GeometryCenter());
                float objDist2 = geomCenter.Distance2(pos.Position());
                if (objDist2 >= Square(shape->GeometrySphere()))
                {
                    obj = nullptr;
                }
                _designatedTarget = obj;
                PoseidonAssert(!dynamic_cast<LaserTarget*>(_designatedTarget.GetLink()));
            }
        }
    }
#if _ENABLE_CHEATS
    if (CHECK_DIAG(DECombat))
    {
        if (odt != _designatedTarget)
        {
            LOG_DEBUG(Physics, "Designated target {}",
                      _designatedTarget ? (const char*)_designatedTarget->GetDebugName() : "<nullptr>");
        }
    }
#endif
}
void LaserTarget::Init(Matrix4Par pos)
{
    CheckDesignatedTarget(pos);
}

void LaserTarget::Move(Matrix4Par transform)
{
    base::Move(transform);
    _lastActivation = Glob.time;
    CheckDesignatedTarget(transform);
}
void LaserTarget::Move(Vector3Par position)
{
    base::Move(position);
    _lastActivation = Glob.time;
    CheckDesignatedTarget(*this);
}

bool LaserTarget::IgnoreObstacle(Object* obstacle, ObjIntersect type) const
{
    if (type == ObjIntersectFire)
    {
        if (obstacle == _designatedTarget)
        {
            return true;
        }
    }
    return base::IgnoreObstacle(obstacle, type);
}

void LaserTarget::Simulate(float deltaT, SimulationImportance prec)
{
    if (IsLocal())
    {
        if (_lastActivation < Glob.time - 10)
        {
            SetDelete();
        }
    }
    base::Simulate(deltaT, prec);
}

void LaserTarget::DrawDiags()
{
#if _ENABLE_CHEATS
    if (CHECK_DIAG(DECombat))
    {
        GScene->DrawCollisionStar(Position(), 0.1, PackedWhite);
        GScene->DrawCollisionStar(AimingPosition(), 0.075, PackedBlack);
    }
#endif
}

void LaserTarget::Draw(int level, ClipFlags clipFlags, const FrameBase& pos)
{
    if (!USER_CONFIG.IsEnabled(DTTracers))
    {
        return;
    }
    if (IsLocal())
    {
        if (_lastActivation < Glob.time - 0.2f)
        {
            return;
        }
    }
    else
    {
        if (_lastActivation < Glob.time - 2.0f)
        {
            return;
        }
    }
    float size = 0.03;

    const Camera& camera = *GScene->GetCamera();

    Vector3 posp = GScene->ScaledInvTransform() * pos.Position();

    float nearest = camera.Near();
    if (posp.Z() < nearest)
    {
        return;
    }

    Matrix4Val project = camera.Projection();

    float invW = 1 / posp[2];
    float sizeX = +project(0, 0) * size * invW * camera.InvLeft();

    if (sizeX < 1)
    {
        Object::DrawPoints(1, clipFlags, pos);
    }
    else
    {
        SetScale(size);
        base::Draw(0, clipFlags, pos);
        SetScale(1);
    }
}

float LaserTarget::VisibleSize() const
{
    // make sure we can detect it on very long distance
    return 40;
}

Vector3 LaserTarget::AimingPosition() const
{
    // drag aiming point nearer to the source
    return Position() - Direction() * 0.5;
}

float LaserTarget::VisibleSizeRequired() const
{
    return 0.01;
}

LSError LaserTarget::Serialize(ParamArchive& ar)
{
    return base::Serialize(ar);
}

NetworkMessageType LaserTarget::GetNMType(NetworkMessageClass cls) const
{
    return base::GetNMType(cls);
}
NetworkMessageFormat& LaserTarget::CreateFormat(NetworkMessageClass cls, NetworkMessageFormat& format)
{
    base::CreateFormat(cls, format);
    return format;
}
TMError LaserTarget::TransferMsg(NetworkMessageContext& ctx)
{
    TMCHECK(base::TransferMsg(ctx));
    return TMOK;
}
float LaserTarget::CalculateError(NetworkMessageContext& ctx)
{
    float error = base::CalculateError(ctx);
    return error;
}

} // namespace Poseidon
