
#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Core/Config/EngineConfig.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/World/World.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/Graphics/Rendering/Lighting/Lights.hpp>
#include <Poseidon/World/Terrain/Landscape.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/Input/InputSubsystem.hpp>

#include <Poseidon/Graphics/Rendering/Draw/SpecLods.hpp>
#include <Poseidon/Foundation/Containers/ConstructTraitsModern.hpp>
#include <cmath>
#include <type_traits>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Containers/StaticArray.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>
#include <Poseidon/Foundation/Math/MathDefs.hpp>
#include <Poseidon/Foundation/Math/MathOpt.hpp>
#include <Poseidon/Foundation/Math/V3Quads.hpp>
#include <Poseidon/Foundation/Memory/MemAlloc.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/Foundation/platform.hpp>

namespace Poseidon
{
using Poseidon::Foundation::MStorage;

#if _KNI
#define PrefetchTemp(a, i) _mm_prefetch((char*)&(a[i]), _MM_HINT_NTA)
#define Prefetch(a, i) _mm_prefetch((char*)&(a[i]), _MM_HINT_T0)
#else
#define PrefetchTemp(a, i)
#define Prefetch(a, i)
#endif

#if USE_QUADS
static StaticStorage<V3Quad> PosTransQStorage;
#endif

static StaticStorage<Vector3> PosTransStorage;

static StaticStorage<TLVertex> TLVertexStorage;

static StaticStorage<ClipFlags> ClipStorage;

void TLVertexTable::DoConstruct()
{
    const int MaxBuf = 4 * 1024;
#if USE_QUADS
#ifdef NDEBUG
    _posTransQ.SetStorage(PosTransQStorage.Init(MaxBuf / 4));
#endif
    _useQuads = false;
#endif
    _posTrans.SetStorage(PosTransStorage.Init(MaxBuf));

    _vert.SetStorage(TLVertexStorage.Init(MaxBuf));
    _clip.SetStorage(ClipStorage.Init(MaxBuf));

    _orHints = ClipHints; // assume worst case
    _andHints = 0;
}

TLVertexTable::TLVertexTable()
{
    DoConstruct();
}

void TLVertexTable::Init(const IAnimator* anim, const Shape& src, const Matrix4& toView)
{
    DoTransformPoints(anim, src, toView);
}

TLVertexTable::TLVertexTable(const IAnimator* anim, const Shape& src, const Matrix4& toView)
{
    DoConstruct();
    DoTransformPoints(anim, src, toView);
}

void TLVertexTable::ReleaseTables()
{
    _clip.Clear();
    _posTrans.Clear();
    _vert.Clear();
}

int TLVertexTable::AddPos()
{
// used for clipping
// interpolation result must be stored somewhere
#if USE_QUADS
    if (_useQuads)
    {
        PoseidonAssert(_posTransQ.Size() == _vert.Size());
        int index = _vert.Size();
        int newSize = index + 1;
        _posTransQ.Resize(newSize);
        _vert.Resize(newSize);
        _clip.Resize(newSize);
        return index;
    }
    else
#endif
    {
        PoseidonAssert(_posTrans.Size() == _vert.Size());
        int index = _vert.Size();
        int newSize = index + 1;
        _posTrans.Resize(newSize);
        _vert.Resize(newSize);
        _clip.Resize(newSize);
        return index;
    }
}

void TLVertexTable::DoTransformPoints(const VertexTable& src, const Matrix4& posView, int beg, int end)
{
    // this is the most common case
    // simple mesh with no decal polygons

#if USE_QUADS
    if (_useQuads)
    {
        src._posQ.Transform(_posTransQ.QuadData(), posView, beg, end);
    }
    else
#endif
    {
        int size = end - beg;
        Vector3* d = _posTrans.Data() + beg;
        const Vector3* s = src._pos.Data() + beg;
        while (--size >= 0)
        {
            (*d++).SetMultiply(posView, *s++);
        }
    }
}

void TLVertexTable::DoTransformPoints(const IAnimator* anim, const Shape& src, const Matrix4& posView)
{
    int i;
    _orHints = src._orHints;
    _andHints = src._andHints;
    int nPos = src._clip.Size();
    _clip.Realloc(nPos);
    _clip.Resize(nPos);
    _vert.Realloc(nPos), _vert.Resize(nPos);
    Poseidon::Foundation::ModernTraits<std::remove_pointer_t<decltype(_clip.Data())>>::CopyData(_clip.Data(),
                                                                                                src._clip.Data(), nPos);
    if ((_orHints & ClipDecalMask) == 0)
    {
#if USE_QUADS
        static bool UseQuads = true;
        if (ENGINE_CONFIG.enablePIII)
        {
#if _ENABLE_CHEATS
            if (InputSubsystem::Instance().GetCheat2ToDo(SDL_SCANCODE_S))
            {
                UseQuads = !UseQuads;
                GlobalShowMessage(500, "SSE T&L %s", UseQuads ? "On" : "Off");
            }
#endif
        }
        if (src._posQ.QuadData() && UseQuads)
        {
            _useQuads = true;
            _posTransQ.Realloc(src._posQ.Size());
            _posTransQ.Resize(src._posQ.Size());
        }
        else
#endif
        {
#if USE_QUADS
            _useQuads = false;
#endif
            _posTrans.Realloc(src._pos.Size());
            _posTrans.Resize(src._pos.Size());
        }

        anim->DoTransform(*this, src, posView, 0, nPos);
        return;
    }

#if USE_QUADS
    _useQuads = false;
#endif
    _posTrans.Realloc(src._pos.Size());
    _posTrans.Resize(src._pos.Size());

    if ((_orHints & ClipDecalMask) == ClipDecalVertical && (_andHints & ClipDecalMask) == ClipDecalVertical)
    {
        // second quite often - whole object is vertical decal (used for trees)
        Matrix4 vDecal(MZero);
        // we want to use only vertical part of rotation
        vDecal.SetUpAndAside(posView.DirectionUp(), VAside);
        vDecal = vDecal * GLOB_SCENE->GetCamera()->ScaleMatrix();
        // decal transformation must have same scale and position as object
        vDecal.SetPosition(posView.Position());
        vDecal.SetScale(posView.Scale());
        // vDecal.SetOrientation(vDecal.Orientation()*Matrix3(MScale,Scale()));
        int size = _posTrans.Size();
        for (i = 0; i < size; i++)
        {
            const V3& pos = src._pos[i];
            // we want to use only vertical part of rotation
            _posTrans[i].SetFastTransform(vDecal, pos);
        }
    }
    else if ((_orHints & ClipDecalMask) == ClipDecalNormal && (_andHints & ClipDecalMask) == ClipDecalNormal)
    {
        // whole object is decal
        Matrix4 decal = GLOB_SCENE->GetCamera()->ScaleMatrix();
        decal.SetPosition(posView.Position());
        decal.SetScale(posView.Scale());
        int size = _posTrans.Size();
        for (i = 0; i < size; i++)
        {
            const V3& pos = src._pos[i];
            // we want to use only vertical part of rotation
            _posTrans[i].SetFastTransform(decal, pos);
        }
    }
    else
    {
        // non-decal part must be handled by anim
        // general case: not very often
        Matrix4 decal(NoInit);
        Matrix4 vDecal(NoInit);
        bool vDecalSet = false;
        bool decalSet = false;
        int size = _posTrans.Size();
        for (i = 0; i < size; i++)
        {
            const V3& pos = src._pos[i];
            ClipFlags clip = src._clip[i];
            V3& dPos = _posTrans[i];
            switch (clip & ClipDecalMask)
            {
                default:
                    dPos.SetFastTransform(posView, pos);
                    break;
                case ClipDecalNormal:
                    if (!decalSet)
                    {
                        decal = GLOB_SCENE->GetCamera()->ScaleMatrix();
                        decal.SetPosition(posView.Position());
                        decal.SetScale(posView.Scale());
                        decalSet = true;
                    }
                    dPos.SetFastTransform(decal, pos);
                    break;
                case ClipDecalVertical:
                    // we want to use only vertical part of rotation
                    // this can be achieved by setting matrix
                    if (!vDecalSet)
                    {
                        // vDecal=Matrix4(MZero);
                        vDecal.SetUpAndAside(posView.DirectionUp(), VAside);
                        vDecal.SetPosition(VZero);
                        vDecal = vDecal * GLOB_SCENE->GetCamera()->ScaleMatrix();
                        vDecal.SetPosition(posView.Position());
                        vDecal.SetScale(posView.Scale());
                        vDecalSet = true;
                    }
                    dPos.SetFastTransform(vDecal, pos);
                    break;
            }
        }
    }
}

inline void VerifyClip(TLVertex* d, float minX, float maxX, float minY, float maxY)
{
#if 1
    saturate(d->pos[0], minX, maxX);
    saturate(d->pos[1], minY, maxY);
#endif
}

void TLVertexTable::DoPerspective(const Camera& camera, ClipFlags clip)
{
#if USE_QUADS
    if (_useQuads)
    {
        // clip flags may become invalid after this operation?
        _posTransQ.Perspective(_vert.Data(), camera.Projection());
    }
    else
#endif
    {
        float minX = GEngine->MinSatX(), maxX = GEngine->MaxSatX();
        float minY = GEngine->MinSatY(), maxY = GEngine->MaxSatY();
        //
        const Matrix4* mc = &camera.Projection();

#define ADJUST_MAT 1
#if !ADJUST_MAT
        float zCoef = 1;
        if (!GEngine->CanZBias())
        {
            int zBias = GEngine->GetBias();
            zCoef = 1.0f - zBias * 1e-6f;
        }
#else
        Matrix4 mcAdjusted;
        if (!GEngine->CanZBias())
        {
            int zBias = GEngine->GetBias();
            if (zBias > 0)
            {
                float zMult = 1, zAdd = 0;
                GEngine->GetZCoefs(zAdd, zMult);

                mcAdjusted = *mc;

                mcAdjusted(2, 2) = mcAdjusted(2, 2) * zMult + zAdd;
                mcAdjusted.SetPosition(
                    Vector3(mcAdjusted.Position().X(), mcAdjusted.Position().Y(), mcAdjusted.Position().Z() * zMult));

                mc = &mcAdjusted;
            }
        }
#endif

        int size = _posTrans.Size();
        TLVertex* d = _vert.Data();
        const Vector3* s = _posTrans.Data();
        // many meshes are not clipped at all - skip check
        if ((clip & ClipAll) == 0)
        {
            while (--size >= 0)
            {
#if _KNI || _VEC4 //|| _T_MATH
                Vector3 dt;
                float rhw = dt.SetPerspectiveProject(*mc, *s);
                (*d).pos[0] = dt[0];
                (*d).pos[1] = dt[1];
                (*d).pos[2] = dt[2];
#else
                float rhw = (*d).pos.SetPerspectiveProject(*mc, *s);
#endif
                d->rhw = rhw;
                VerifyClip(d, minX, maxX, minY, maxY);
#if !ADJUST_MAT
                d->pos[2] *= zCoef;
#endif
                d++, s++;
                // check z bias coef
            }
        }
        else
        {
            const ClipFlags* c = _clip.Data();
            while (--size >= 0)
            {
                if (((*c) & ClipAll) == ClipNone)
                {
#if _KNI || _VEC4 //|| _T_MATH
                    Vector3 dt;
                    float rhw = dt.SetPerspectiveProject(*mc, *s);
                    (*d).pos[0] = dt[0];
                    (*d).pos[1] = dt[1];
                    (*d).pos[2] = dt[2];
#else
                    float rhw = (*d).pos.SetPerspectiveProject(*mc, *s);
#endif
                    d->rhw = rhw;
                    VerifyClip(d, minX, maxX, minY, maxY);
#if !ADJUST_MAT
                    d->pos[2] *= zCoef;
#endif
                }
                c++, d++, s++;
            }
        }
    }
}

// Constant-fog gating splits cleanly across categories:
//   Routing::FogDisabled  — caller says "skip fog for this draw"
//   Backend::IsAlphaFog   — draw uses alpha-blend as its fog visual
// The composite check returns "no fog" only when FogDisabled is set
// AND IsAlphaFog is not — matching the legacy int mask
// `(spec & (FogDisabled | IsAlphaFog)) == FogDisabled`.
inline float ConstantFog(int spec)
{
    const render::LegacySpec specT = render::SplitLegacy(spec);
    if (render::Has(specT.routing, render::Routing::FogDisabled) &&
        !render::Has(specT.backend, render::Backend::IsAlphaFog))
    {
        return 0;
    }
    return GScene->GetConstantFog();
}

inline int ConstantFogInt(int spec)
{
    const render::LegacySpec specT = render::SplitLegacy(spec);
    if (render::Has(specT.routing, render::Routing::FogDisabled) &&
        !render::Has(specT.backend, render::Backend::IsAlphaFog))
    {
        return 0;
    }
    return toIntFloor(GScene->GetConstantFog() * 255);
}

const float StartLights = 0.01; // NightEffect when lights start to be visible

#define DP(a, b) ((a).X() * (b).X() + (a).Y() * (b).Y() + (a).Z() * (b).Z())
#define SIZE2(x, y, z) (Square(x) + Square(y) + Square(z))

inline PackedColor PackedColor255(ColorVal color, int a = 255)
{
    int r = toInt(color.R());
    int g = toInt(color.G());
    int b = toInt(color.B());
    saturate(r, 0, 255);
    saturate(g, 0, 255);
    saturate(b, 0, 255);
    return PackedColor((a << 24) | (r << 16) | (g << 8) | b);
}

void TLVertexTable::DoMaterialLightingP(const TLMaterial& mat, const Matrix4& worldToModel, const LightList& lights,
                                        const VertexTable& mesh, int beg, int end)
{
    if (end <= beg)
    {
        return; // safety: nothing to light
    }

    // perform lighting
    // first of all apply directional light to all normals
    // this can be done at TLVertexTable level
    LightSun* sun = GScene->MainLight();
    float addLightsFactor = sun->NightEffect();
    if ((mat.specFlags & DisableSun) == 0)
    {
        sun->SetMaterial(mat);
        if (lights.Size() > 0)
        {
            TLMaterial temp;
            temp.ambient = mat.ambient * addLightsFactor;
            temp.diffuse = mat.diffuse * addLightsFactor;
            temp.specFlags = mat.specFlags;
            temp.emmisive = mat.emmisive;
            temp.forcedDiffuse = mat.forcedDiffuse * addLightsFactor;
            for (int index = 0; index < lights.Size(); index++)
            {
                lights[index]->SetMaterial(temp);
                lights[index]->Prepare(worldToModel);
            }
        }
    }
    else
    {
        addLightsFactor = 1;

        TLMaterial temp;
        temp.ambient = HBlack;
        temp.diffuse = HBlack;
        temp.emmisive = HBlack;
        temp.forcedDiffuse = HBlack;
        temp.specFlags = mat.specFlags;

        sun->SetMaterial(temp);
        if (lights.Size() > 0)
        {
            for (int index = 0; index < lights.Size(); index++)
            {
                lights[index]->SetMaterial(mat);
                lights[index]->Prepare(worldToModel);
            }
        }
    }

    const Camera* camera = GScene->GetCamera();
    Matrix4Val invScale = camera->InvScaleMatrix();

    // normal lighting
    // for all vertices in mesh calculate positional lights and fog
    // check for special case: no lights
    bool someLights = (addLightsFactor >= StartLights && lights.Size() > 0);
    // assume dammage value is constant over whole object
    Vector3 sunDirection = worldToModel.Rotate(sun->Direction());
    sunDirection.Normalize();

    Color diffuse = sun->DiffusePrecalc();
    Color ambient = sun->AmbientPrecalc() + mat.emmisive;

    int spec = mat.specFlags;
    int aFactor = 0x100;
    if (spec & IsColored)
    {
        aFactor = toInt(GScene->GetConstantColor().A() * 0x100);
        saturate(aFactor, 0, 0x100);
    }

    int fogValue = ConstantFogInt(spec);
    if (!someLights)
    {
        if (fogValue >= 0)
        {
            PackedColor packedSpecular; // HW fog
            PackedColor packedAmbient;
            if (spec & IsAlphaFog)
            {
                // consider alpha from constant color
                fogValue = 0xff - fogValue; // IsAlphaFog
                fogValue = (fogValue * aFactor) >> 8;
                packedAmbient = PackedColorRGB(ambient, fogValue);
                packedSpecular = PackedBlack;
            }
            else
            {
                packedAmbient = PackedColorRGB(ambient, 0xff);
                packedSpecular = PackedColorRGB(PackedBlack, 0xff - fogValue);
                fogValue = 0xff;
            }

            // near objects do not need any fog
            // perform DirectX Lighting

            TLVertex* v = VertexData() + beg;
            const Vector3* norm = &mesh.Norm(beg);
            const UVPair* tex = mesh._tex.Data() + beg;

            ambient = ambient * 255; // pre-multiply to correct scale
            diffuse = diffuse * 255;
            for (int i = end - beg; --i >= 0;)
            {
#if _KNI
                Coord cosFi = (*norm).DotProduct(sunDirection);
                saturateMax(cosFi, 0);
                _mm_prefetch((char*)(norm + 4), _MM_HINT_NTA);
                Color color = ambient + diffuse * cosFi;
                v->color = PackedColor255(color, fogValue);
#elif __ICL
                Coord cosFi = *norm * sunDirection;
                saturateMax(cosFi, 0);
                Color color = ambient + diffuse * cosFi;
                v->color = PackedColor255(color, fogValue);
#else
                Coord cosFi = *norm * sunDirection;
                if (cosFi > 0)
                {
                    Color color = ambient + diffuse * cosFi;
                    v->color = PackedColor255(color, fogValue);
                }
                else
                {
                    v->color = packedAmbient;
                }
#endif
                v->specular = packedSpecular;
                v->t0 = *tex;
                norm++;
                v++;
                tex++;
            }
        }
        else
        {
            PackedColor packedAmbient = PackedColor(ambient);
            // calculate per vertex fog
            const UVPair* tex = mesh._tex.Data();
            for (int i = beg; i < end; i++)
            {
                TLVertex& v = SetVertex(i);
                Vector3Val scalePos = TransPos(i);
                float dist2 = SIZE2(scalePos.X() * invScale(0, 0), scalePos.Y() * invScale(1, 1), scalePos.Z());
                int fog = GScene->Fog8(dist2);
                // alpha is assumed 1 - this is always true for normal lighting
                PackedColor specular;
                if (spec & IsAlphaFog)
                {
                    fog = 0xff - fog; // IsAlphaFog
                    fog = (fog * aFactor) >> 8;
                    specular = PackedBlack;
                }
                else
                {
                    specular = PackedColorRGB(PackedBlack, 0xff - fog);
                    fog = 0xff;
                }

#if __ICL
                Coord cosFi = sunDirection * mesh.Norm(i);
                saturateMax(cosFi, 0);
                Color color = ambient + diffuse * cosFi;
                v.color = PackedColorRGB(color, fog);
#else
                Coord cosFi = sunDirection * mesh.Norm(i);
                if (cosFi > 0)
                {
                    Color color = ambient + diffuse * cosFi;
                    v.color = PackedColorRGB(color, fog);
                }
                else
                {
                    v.color = PackedColorRGB(packedAmbient, fog);
                }
#endif
                v.specular = specular;
                v.t0 = tex[i];
                // check if there are not some unsupported lighting flags
            }
        }
    }
    else
    {
        const UVPair* tex = mesh._tex.Data();
        for (int i = beg; i < end; i++)
        {
            TLVertex& v = SetVertex(i);
            Coord cosFi = sunDirection * mesh.Norm(i);
            Color colorI = ambient;
#if __ICL
            saturateMax(cosFi, 0);
            colorI += diffuse * cosFi;
#else
            if (cosFi > 0)
            {
                colorI += diffuse * cosFi;
            }
#endif

            Vector3Val norm = mesh.Norm(i);
            Vector3Val pos = mesh.Pos(i);
            for (int index = 0; index < lights.Size(); index++)
            {
                colorI += lights[index]->Apply(pos, norm);
            }
            // lighting with normal defined
            // check if there are not some unsupported lighting flags
            int fog = fogValue;
            if (fogValue < 0)
            {
                Vector3Val scalePos = TransPos(i);
                float dist2 = SIZE2(scalePos.X() * invScale(0, 0), scalePos.Y() * invScale(1, 1), scalePos.Z());
                fog = GScene->Fog8(dist2);
            }

            PackedColor specular;
            if (spec & IsAlphaFog)
            {
                fog = 0xff - fog; // IsAlphaFog
                fog = (fog * aFactor) >> 8;
                v.color = PackedColorRGB(colorI, fog);
                v.specular = PackedBlack;
            }
            else
            {
                v.color = PackedColorRGB(colorI, 0xff);
                v.specular = PackedColorRGB(PackedBlack, 0xff - fog);
            }
            v.t0 = tex[i];

            // alpha is assumed 1 - this is always true for normal lighting
        }
    }
}

void TLVertexTable::DoSkyLighting(const VertexTable& mesh, int spec)
{
    LightSun* sun = GScene->MainLight();
    Vector3Val sunDir = sun->SunDirection();
    int i;
    // for all vertices in mesh calculate positional lights and fog
    Color accom = GEngine->GetAccomodateEye();
    const int size = NVertex();
    const UVPair* tex = mesh._tex.Data();
    PoseidonAssert(_posTrans.Size() == size);
    for (i = 0; i < size; i++)
    {
        TLVertex& v = SetVertex(i);

        // there is very light area around the sun
        // this could be simulated with specular highlights
        Vector3Val direction = mesh.Norm(i);
        float cosSun = -sunDir * direction;
        saturateMax(cosSun, 0);
        float cosSun2 = cosSun * cosSun;
        float cosSun4 = cosSun2 * cosSun2;
        float sunCoef = cosSun4 * cosSun2;
#if 1
        Color colorI = sun->SunSkyColor() * sunCoef + sun->SkyColor();
#else
        // debugging: orange sky
        Color colorI(1, 0.5, 0);
#endif

        PoseidonAssert(spec & IsAlphaFog);
        v.color = PackedColorRGB(colorI * accom, 255);
        v.specular = PackedBlack;
        v.t0 = tex[i];
    }
}

void TLVertexTable::DoStarLighting(const Matrix4& worldToModel, const VertexTable& mesh, int spec)
{
    LightSun* sun = GScene->MainLight();
    int i;
    Landscape* land = GScene->GetLandscape();
    // calculate common alpha correction
    float alphaCorrection = (
        // overcast limitation
        (1.5 * land->SkyThrough() - 0.5) *
        // daytime limitation
        sun->StarsVisibility() *
        // resolution correction
        (GEngine->Width() * GEngine->Height()) * (1.0 / (640 * 480)) *
        // zoom correction
        GScene->GetCamera()->InvLeft());

    Vector3 sunDirection = worldToModel.Rotate(sun->Direction());
    sunDirection.Normalize();

    Color accom = GEngine->GetAccomodateEye();
    const int size = NVertex();
    const UVPair* tex = mesh._tex.Data();
    for (i = 0; i < size; i++)
    {
        TLVertex& v = SetVertex(i);

        // calculate angle from sun
        Vector3Val sPos = mesh.Pos(i);
        float cosSun = -sPos * sunDirection;
        if (cosSun > 0)
        {
            //(starPos is spherical, SunDirection is normalized)
            float invSize = sPos.InvSize();
            cosSun = cosSun * invSize;
            float cosSun2 = cosSun * cosSun;
            cosSun = cosSun2 * cosSun2;
        }
        else
        {
            cosSun = 0;
        }
        // use different brightness for different stars
        ClipFlags clip = Clip(i);

        int user = clip & ClipUserMask;
        float brightness = user * (1.0 / MaxUserValue / ClipUserStep);
        // note: point brightness is calculated for resolution 640x480
        // correct it for finer resolutions
        float a = alphaCorrection * brightness * (1 - cosSun);
        saturate(a, 0, 1);
        accom.SetA(a);
        v.color = PackedColor(accom);
        v.specular = PackedBlack;
        v.t0 = tex[i];
    }
}

void TLVertexTable::DoLineLighting(const VertexTable& mesh, int spec)
{
    int i;
    Matrix4Val invScale = GScene->GetCamera()->InvScaleMatrix();
    // common alpha correction
    float alphaCorrection = (
        // resolution correction
        GEngine->Width() * (1.0 / 640) *
        // zoom correction
        GScene->GetCamera()->InvLeft());

    Color lineColor = GScene->GetConstantColor();
    float width = lineColor.A(); // width (in m?)

    PoseidonAssert((GetAndHints() & ClipLightMask) == ClipLightLine);

    ColorVal accom = GEngine->GetAccomodateEye();
    const int size = NVertex();
    const UVPair* tex = mesh._tex.Data();
    PoseidonAssert(_posTrans.Size() == size);
    for (i = 0; i < size; i++)
    {
        TLVertex& v = SetVertex(i);
        Vector3Val scalePos = TransPos(i);
        float dist2 = SIZE2(scalePos.X() * invScale(0, 0), scalePos.Y() * invScale(1, 1), scalePos.Z());
        // scale alpha with distance
        // correct it for finer resolutions
        float invCoef = 100.0 / 3;
        if (dist2 > Square(3))
        {
            invCoef = 100 * InvSqrt(dist2);
        }
        float a = alphaCorrection * width * invCoef;
        saturate(a, 0, 1);
        lineColor.SetA(a);
        v.color = PackedColor(lineColor * accom);
        v.specular = PackedBlack;
        v.t0 = tex[i];
    }
}

void TLVertexTable::DoCloudLighting(const VertexTable& mesh, int spec)
{
    Landscape* land = GLandscape;
    Matrix4Val invScale = GScene->GetCamera()->InvScaleMatrix();
    LightSun* sun = GScene->MainLight();
    ColorVal accom = GEngine->GetAccomodateEye();
    // for all vertices in mesh calculate positional lights and fog
    Color colorI;
    float factor = 1.0, alpha = 1.0;
    if (land)
    {
        alpha = land->CloudsAlpha(), factor = land->CloudsBrightness();
    }
    TLMaterial mat;
    mat.diffuse = accom;
    mat.ambient = accom;
    mat.emmisive = HBlack;
    mat.forcedDiffuse = HBlack;
    mat.specFlags = 0;
    sun->SetMaterial(mat);
    colorI = sun->FullResult(1.0) * factor;
    colorI.SetA(alpha);
    colorI = colorI * accom;
    PackedColor color(colorI);
    PoseidonAssert((GetAndHints() & ClipFogMask) == ClipFogSky);
    const int size = NVertex();
    const UVPair* tex = mesh._tex.Data();
    PoseidonAssert(_posTrans.Size() == size);

    float fog = ConstantFog(spec);
    bool constFog = (fog >= 0);

    PoseidonAssert(spec & IsAlphaFog);
    if (constFog)
    {
        int colorAInt = toIntFloor((1 - fog) * colorI.A() * 255);
        saturate(colorAInt, 0, 255);
        color.SetA8(colorAInt);
        for (int i = 0; i < size; i++)
        {
            TLVertex& v = SetVertex(i);
            v.color = color;
            v.specular = PackedBlack;
            v.t0 = tex[i];
        }
    }
    else
    {
        for (int i = 0; i < size; i++)
        {
            TLVertex& v = SetVertex(i);
            Vector3Val scalePos = TransPos(i);
            float dist2 = SIZE2(scalePos.X() * invScale(0, 0), scalePos.Y() * invScale(1, 1), scalePos.Z());

            float fog = GScene->SkyFog8(dist2) * (1.0 / 255);

            float a = (1 - fog) * colorI.A();
            int aInt = toIntFloor(a * 255);
            saturate(aInt, 0, 255);
            v.color = PackedColorRGB(color, aInt);
            v.specular = PackedBlack;
            v.t0 = tex[i];
        }
    }
}

void TLVertexTable::DoShadowLighting(const VertexTable& mesh, int spec)
{
    Matrix4Val invScale = GScene->GetCamera()->InvScaleMatrix();
    // shadows need not be lighted
    int shadowFactorI = GEngine->GetShadowFactor();
    const int size = NVertex();
    PoseidonAssert(_posTrans.Size() == size);
    const UVPair* tex = mesh._tex.Data();
    for (int i = 0; i < size; i++)
    {
        TLVertex& v = SetVertex(i);
        Vector3Val scalePos = TransPos(i);
        float dist2 = SIZE2(scalePos.X() * invScale(0, 0), scalePos.Y() * invScale(1, 1), scalePos.Z());
        int fog = GScene->ShadowFog8(dist2);
        // fog==1 means invisible
        int a = (shadowFactorI * (255 - fog)) >> 8; // a==0 means invisible
        saturate(a, 0, 255);
        v.color = PackedColorRGB(PackedBlack, a); // shadow is black
        v.specular = PackedBlack;
        v.t0 = tex[i];
    }
}

void TLVertexTable::DoLightLighting(const VertexTable& mesh, int spec)
{
    float addLightsFactor = GScene->MainLight()->NightEffect();
    Matrix4Val invScale = GScene->GetCamera()->InvScaleMatrix();
    int i;
    Color accom = GEngine->GetAccomodateEye();
    if (spec & IsColored)
    {
        accom = accom * GScene->GetConstantColor();
    }
    PackedColor lColor(accom);
    const int size = NVertex();
    const UVPair* tex = mesh._tex.Data();
    PoseidonAssert(_posTrans.Size() == size);
    for (i = 0; i < size; i++)
    {
        TLVertex& v = SetVertex(i);
        ClipFlags clip = Clip(i);
        Vector3Val scalePos = TransPos(i);
        float dist2 = SIZE2(scalePos.X() * invScale(0, 0), scalePos.Y() * invScale(1, 1), scalePos.Z());
        float fog = GScene->Fog8(dist2) * (1.0 / 255);
        int mat = clip & ClipUserMask;
        if (mat != static_cast<int>(MSShining) * static_cast<int>(ClipUserStep))
        {
            const float lightFactor = 1.0 / 16;
            fog = (1 - lightFactor) + lightFactor * fog;
        }
        float visibility = addLightsFactor;
        float a = (1 - fog) * visibility;
        int ai = toInt(a * 255);
        saturate(ai, 0, 255);
        v.color = PackedColorRGB(lColor, ai);
        v.specular = PackedBlack;
        v.t0 = tex[i];
    }
}

void TLVertexTable::DoLightingColorized(const LightList& lights,
                                        const PackedColor* colors, // color array
                                        const VertexTable& mesh, int spec)
{
    const int size = NVertex();

    ColorVal accom = GEngine->GetAccomodateEye();
    // landscape lighting
    int i;
    Matrix4Val invScale = GScene->GetCamera()->InvScaleMatrix();
    float addLightsFactor = GScene->MainLight()->NightEffect();
    bool someLights = (addLightsFactor >= StartLights && lights.Size() > 0);

    TLMaterial mat;
    Color ccolor = HWhite;
    ccolor = ccolor * accom;
    mat.diffuse = ccolor;
    mat.ambient = ccolor;
    mat.emmisive = HBlack;
    mat.forcedDiffuse = HBlack;
    mat.specFlags = spec;

    LightSun* sun = GScene->MainLight();
    sun->SetMaterial(mat);
    if (someLights)
    {
        mat.diffuse = mat.diffuse * addLightsFactor;
        mat.ambient = mat.ambient * addLightsFactor;
        for (int index = 0; index < lights.Size(); index++)
        {
            lights[index]->Prepare(MIdentity);
            lights[index]->SetMaterial(mat);
        }
    }

    Vector3 sunDirection = sun->Direction();
    sunDirection.Normalize();

    const UVPair* tex = mesh._tex.Data();
    for (i = 0; i < size; i++)
    {
        TLVertex& v = SetVertex(i);
        Vector3Val scalePos = TransPosA(i);
        float dist2 = SIZE2(scalePos.X() * invScale(0, 0), scalePos.Y() * invScale(1, 1), scalePos.Z());

        Vector3Val normal = mesh.Norm(i);
        Coord cosFi = sunDirection * normal;
        saturateMax(cosFi, 0);
        Color colorI = (sun->DiffusePrecalc() * cosFi + sun->AmbientPrecalc());
        if (someLights)
        {
            // note: Colorized lighting models are always in world space
            Vector3Val pos = mesh.Pos(i);
            for (int index = 0; index < lights.Size(); index++)
            {
                colorI += lights[index]->Apply(pos, normal);
            }
        }
        // lighting with normal defined
        // ClipVarMask value is index into colorization palette
        ClipFlags clip = Clip(i);
        int index = (clip & ClipUserMask) / ClipUserStep;
        PoseidonAssert(index >= 0 && index < 256);
        Color colorize = colors[index];
        colorI = colorI * colorize * 2;
        // check if there are not some unsupported lighting flags
        int fog = GScene->Fog8(dist2);
        if (spec & IsWater)
        {
            fog = 0x80 - (fog >> 1);
        }

        // alpha is assumed 1 - this is always true for normal lighting
        v.color = PackedColorRGB(colorI, 0xff);
        v.specular = PackedColorRGB(PackedBlack, 0xff - fog);
        const UVPair& uv = tex[i];
        v.t0 = uv;
        v.t1.u = uv.u * 32;
        v.t1.v = uv.v * 32;
    }
}

void TLVertexTable::DoLighting(const IAnimator* matSource, const Matrix4& worldToModel, const LightList& lights,
                               const Shape& mesh, int spec)
{
    // note: worldToModel matrix is transformation from world space to model space
    // it is used to transform lights before applying them
    // so that lights can be applied in model space
    const int size = NVertex();

    if (size <= 0)
    {
        return; // nothing to light
    }

    if ((spec & (IsShadow | IsLight)) == 0 &&
        (GetOrHints() & (ClipLightMask | ClipFogMask)) == (ClipLightNormal | ClipFogNormal))
    {
        // and store them in Shape
        // note: User data contain material index
        int orMaterial = GetOrHints() & ClipUserMask;
        int andMaterial = GetAndHints() & ClipUserMask;
        if (orMaterial == andMaterial)
        {
            int material = andMaterial / ClipUserStep;

            matSource->DoLight(*this, mesh, worldToModel, lights, spec, material, 0, size);
            return;
        }
    }

    if (spec & IsShadow)
    { // shadows need not be lighted
        DoShadowLighting(mesh, spec);
        return;
    }
    else if ((spec & (IsLight | SpecLighting)) == IsLight)
    { // volumetric lights and flares need not be lighted
        DoLightLighting(mesh, spec);
        return;
    }
    else
    {
        PoseidonAssert((spec & IsWater) == 0);
        ClipFlags globalLight = GetAndHints() & ClipLightMask;
        if (globalLight != (GetOrHints() & ClipLightMask))
        {
            globalLight = 0;
        }

        switch (globalLight)
        {
            case ClipLightCloud:
                DoCloudLighting(mesh, spec);
                return;
            case ClipLightSky:
                DoSkyLighting(mesh, spec);
                return;
            case ClipLightStars:
                DoStarLighting(worldToModel, mesh, spec);
                return;
            case ClipLightLine:
                DoLineLighting(mesh, spec);
                return;
        }

        if ((spec & SpecLighting) == 0)
        {
            ClipFlags lastMode = 0; // normal lighting flags
            int lastModeBeg = 0, lastModeEnd = 0;

            // normal lighting
            // for all vertices in mesh calculate positional lights and fog
            for (int v = 0; v < size; v++)
            {
                ClipFlags clip = Clip(v);
                ClipFlags lightFlags = clip & ClipUserMask;

                // check if same mode as it was before
                if (lightFlags == lastMode)
                {
                    // same lighting as before - extend range
                    lastModeEnd = v + 1;
                }
                else
                {
                    // light mode changed
                    if (lastModeEnd > lastModeBeg)
                    {
                        int material = lastMode / ClipUserStep;
                        matSource->DoLight(*this, mesh, worldToModel, lights, spec, material, lastModeBeg, lastModeEnd);
                    }
                    lastModeBeg = v;
                    lastModeEnd = v + 1;
                    lastMode = lightFlags;
                }
            } // for all vertices
            if (lastModeEnd > lastModeBeg)
            {
                int material = lastMode / ClipUserStep;
                matSource->DoLight(*this, mesh, worldToModel, lights, spec, material, lastModeBeg, lastModeEnd);
            }
            return;
        }
        else
        {
            LightSun* sun = GScene->MainLight();
            Color accom = GEngine->GetAccomodateEye();

            Landscape* land = GScene->GetLandscape();
            if (spec & IsColored)
            {
                accom = accom * Color(GScene->GetConstantColor());
            }
            accom.SetA(1);

            // special (sun or moon) lighting
            const UVPair* tex = mesh._tex.Data();
            for (int i = 0; i < size; i++)
            {
                TLVertex& v = SetVertex(i);
                ClipFlags clip = Clip(i);
                ClipFlags lightFlags = clip & ClipLightMask;

                Color colorI;
                switch (lightFlags)
                {
                    case ClipLightSun:
                        colorI = sun->SunObjectColor() * accom;
                        break;
                    case ClipLightSunHalo:
                        colorI = sun->SunHaloObjectColor() * accom;
                        break;
                    case ClipLightMoon:
                        colorI = sun->MoonObjectColor() * accom;
                        break;
                    case ClipLightMoonHalo:
                        colorI = sun->MoonHaloObjectColor() * accom;
                        break;
                    default:
                        // NoLight
                        colorI = HWhite;
                        Fail("Invalid light mask");
                        break;
                }
                if (land)
                {
                    colorI.SetA(colorI.A() * land->SkyThrough());
                }

                v.color = PackedColor(colorI);
                v.specular = PackedBlack;
                v.t0 = tex[i];
            }
        }
    }
}

__forceinline ClipFlags NeedsClippingInline(Vector3Par point, const Camera& camera)
{
    // check _posTrans for all six clipping planes
    float cNear = camera.ClipNear();
    float cFar = camera.ClipFar();
    ClipFlags flags = ClipNone;
    if (point.Z() < cNear)
    {
        flags |= ClipFront; // front clipping plane
    }
    if (point.Z() > cFar)
    {
        flags |= ClipBack; // back clipping plane
    }

    if (point.Z() < -point.X())
    {
        flags |= ClipLeft; // left clipping plane
    }
    if (point.Z() < +point.X())
    {
        flags |= ClipRight; // right clipping plane
    }

    if (point.Z() < -point.Y())
    {
        flags |= ClipTop; // top clipping plane
    }
    if (point.Z() < +point.Y())
    {
        flags |= ClipBottom; // bottom clipping plane
    }
    if (camera.IsUserClip())
    {
        // check user clipping plane
        if (point * camera.UserClipDir() + camera.UserClipVal() < 0)
        {
            flags |= ClipUser0;
        }
    }
    return flags;
}

ClipFlags NeedsClipping(Vector3Par point, const Camera& camera)
{
    return NeedsClippingInline(point, camera);
}

ClipFlags NeedsClipping(const Matrix4& transform, const Camera& camera, Vector3Val point)
{
    Point3 tPoint(VFastTransform, transform, point);
    return NeedsClipping(tPoint, camera);
}

#define STATS 0

ClipFlags TLVertexTable::CheckClipping(const Camera& camera, ClipFlags mask, ClipFlags& andClip)
{
    int size = _clip.Size();
#if STATS
    static int Done, Skipped;
    static int ClippedAll, ClippedNone;
    int count = size;
    int maxCount = 100000;
    // int count = 1;
    // int maxCount = 1000;
#endif
    andClip = mask;
    if (!mask)
    {
        for (int i = 0; i < size; i++)
        {
            _clip[i] &= ClipHints | ClipUserMask;
        }
#if STATS
        Skipped += count;
#endif
        return ClipNone;
    }
    else
    {
        ClipFlags orClip = ClipNone;
#if USE_QUADS
        if (_useQuads)
        {
            for (int i = 0; i < size; i++)
            {
                ClipFlags clip = _clip[i];
#if _T_MATH
                const V3QElement& p = _posTransQ[i];
                const Vector3 point(p.X(), p.Y(), p.Z());
#else
                const V3& point = _posTransQ[i];
#endif
                ClipFlags hints = clip & (ClipHints | ClipUserMask);
                clip &= NeedsClippingInline(point, camera) & mask;
                orClip |= clip;
                andClip &= clip;
                _clip[i] = clip | hints;
            }
        }
        else
#endif
        {
            for (int i = 0; i < size; i++)
            {
                ClipFlags clip = _clip[i];
                const V3& point = _posTrans[i];
                ClipFlags hints = clip & (ClipHints | ClipUserMask);
                ClipFlags clipRes = NeedsClippingInline(point, camera);
                clip &= clipRes & mask;
                orClip |= clip;
                andClip &= clip;
                _clip[i] = clip | hints;
            }
        }
#if STATS
        if (andClip)
        {
            ClippedAll += count;
            /*
            if (count>100)
            {
                LOG_DEBUG(Graphics, "Late full clipping {}",count);
            }
            */
        }
        else if (!orClip)
        {
            ClippedNone += count;
            /*
            if (count>100)
            {
                LOG_DEBUG(Graphics, "Late no clipping {}",count);
            }
            */
        }
        Done += count;
        if (Done >= maxCount)
        {
            LOG_DEBUG(Graphics, "Skipped ratio {:.2f}, noClip {:.2f}, fullClip {:.2f}",
                      (float)Skipped / (Done + Skipped), (float)ClippedNone / Done, (float)ClippedAll / Done);
            Done = Skipped = 0;
            ClippedAll = ClippedNone = 0;
        }
#endif
        return orClip;
    }
}

TLMaterial::TLMaterial() : specular(HBlack), specularPower(0) {}

bool TLMaterial::IsEqual(const TLMaterial& src) const
{
    if (specFlags != src.specFlags)
    {
        return false;
    }

    if (fabs(ambient.R() - src.ambient.R()) > 0.001)
    {
        return false;
    }
    if (fabs(ambient.G() - src.ambient.G()) > 0.001)
    {
        return false;
    }
    if (fabs(ambient.B() - src.ambient.B()) > 0.001)
    {
        return false;
    }
    if (fabs(ambient.A() - src.ambient.A()) > 0.001)
    {
        return false;
    }

    if (fabs(diffuse.R() - src.diffuse.R()) > 0.001)
    {
        return false;
    }
    if (fabs(diffuse.G() - src.diffuse.G()) > 0.001)
    {
        return false;
    }
    if (fabs(diffuse.B() - src.diffuse.B()) > 0.001)
    {
        return false;
    }
    if (fabs(diffuse.A() - src.diffuse.A()) > 0.001)
    {
        return false;
    }

    if (fabs(forcedDiffuse.R() - src.forcedDiffuse.R()) > 0.001)
    {
        return false;
    }
    if (fabs(forcedDiffuse.G() - src.forcedDiffuse.G()) > 0.001)
    {
        return false;
    }
    if (fabs(forcedDiffuse.B() - src.forcedDiffuse.B()) > 0.001)
    {
        return false;
    }
    if (fabs(forcedDiffuse.A() - src.forcedDiffuse.A()) > 0.001)
    {
        return false;
    }

    if (fabs(emmisive.R() - src.emmisive.R()) > 0.001)
    {
        return false;
    }
    if (fabs(emmisive.G() - src.emmisive.G()) > 0.001)
    {
        return false;
    }
    if (fabs(emmisive.B() - src.emmisive.B()) > 0.001)
    {
        return false;
    }
    if (fabs(emmisive.A() - src.emmisive.A()) > 0.001)
    {
        return false;
    }

    if (fabs(specular.R() - src.specular.R()) > 0.001)
    {
        return false;
    }
    if (fabs(specular.G() - src.specular.G()) > 0.001)
    {
        return false;
    }
    if (fabs(specular.B() - src.specular.B()) > 0.001)
    {
        return false;
    }
    if (fabs(specular.A() - src.specular.A()) > 0.001)
    {
        return false;
    }

    return true;
}

void CreateMaterialNormal(TLMaterial& mat, ColorVal col)
{
    mat.emmisive = HBlack;
    mat.ambient = col;
    mat.diffuse = col;
    mat.forcedDiffuse = HBlack;
    mat.specFlags = 0; // fog mode and some more special flags
}

void CreateMaterialShining(TLMaterial& mat, ColorVal col)
{
    mat.emmisive = col;
    mat.ambient = HBlack;
    mat.diffuse = HBlack;
    mat.forcedDiffuse = HBlack;
    mat.specFlags = 0; // fog mode and some more special flags
}

void CreateMaterialConstant(TLMaterial& mat, ColorVal col, float factor)
{
    mat.ambient = col;
    mat.forcedDiffuse = col * factor * 0.5;
    mat.diffuse = col * factor * 0.5;
    mat.diffuse.SetA(col.A());
    mat.emmisive = HBlack;
    mat.specFlags = 0;
}

#define COLOR_DEBUG_MAT 0

inline Color ModulateRGB(Color col, float f)
{
    return col * Color(f, f, f, 1);
}

void CreateMaterial(TLMaterial& mat, ColorVal col, int type)
{
    if (type == 0)
    {
// most common case
#if COLOR_DEBUG_MAT
        CreateMaterialNormal(mat, Color(1, 1, 0));
#else
        CreateMaterialNormal(mat, col);
#endif
        return;
    }
    switch (type)
    {
        case MSShining:
#if COLOR_DEBUG_MAT
            CreateMaterialNormal(mat, Color(0, 1, 1));
#else
            CreateMaterialShining(mat, col);
#endif
            break;
        case MSInShadow:
#if COLOR_DEBUG_MAT
            CreateMaterialNormal(mat, Color(1, 0, 0));
#else
            CreateMaterialConstant(mat, col, 0);
#endif
            break;
        case MSHalfLighted:
#if COLOR_DEBUG_MAT
            CreateMaterialNormal(mat, Color(0, 1, 0));
#else
            CreateMaterialConstant(mat, col, 0.5);
#endif
            break;
        case MSFullLighted:
#if COLOR_DEBUG_MAT
            CreateMaterialNormal(mat, Color(0, 0, 1));
#else
            CreateMaterialConstant(mat, col, 1);
#endif
            break;
        case MSInside:
            CreateMaterialConstant(mat, col, 0.2);
            break;
        case MSInShadow75:
            CreateMaterialConstant(mat, ModulateRGB(col, 0.75f), 0);
            break;
        case MSInside75:
            CreateMaterialConstant(mat, ModulateRGB(col, 0.75f), 0.2);
            break;
        case MSInShadow50:
            CreateMaterialConstant(mat, ModulateRGB(col, 0.50f), 0);
            break;
        case MSInside50:
            CreateMaterialConstant(mat, ModulateRGB(col, 0.50f), 0.2);
            break;
        default:
#if COLOR_DEBUG_MAT
            CreateMaterialNormal(mat, Color(1, 1, 0));
#else
            CreateMaterialNormal(mat, col);
#endif
            break;
    }
}
} // namespace Poseidon
