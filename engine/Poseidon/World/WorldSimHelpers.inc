// Shared helpers for World.cpp — internal linkage only (safe to duplicate)

extern bool DisableTextures;
static bool timeJumpPreview = false;

#define LOG_ADD_REMOVE_VEHICLE 0
#define PERF_SIM 0

// Forward declarations for functions defined in WorldSetup.cpp
bool IsPlayerDead();
bool BreakIntro();
void ShowCinemaBorder(bool show);
ChatChannel ActualChatChannel();
void SetChatChannel(ChatChannel channel);
void PrevChatChannel();
void NextChatChannel();
#if _ENABLE_CHEATS
void DebugOperMap();
void DebugOperMap(AIUnit* unit);
void DebugOperMapTrouble();
extern bool forceControlsPaused;
#endif
extern bool showCinemaBorder;

const float ZoomSpeed = 8;

static void KeepNZone(float& value, float cursor, float pos, float size, float fov)
{
    float minNZone = pos - size;
    float maxNZone = pos + size;
    if (cursor < minNZone)
    {
        value += (minNZone - cursor) * fov;
    }
    else if (cursor > maxNZone)
    {
        value -= (cursor - maxNZone) * fov;
    }
}

inline Matrix3 CameraChange(float heading, float dive)
{
    return Matrix3(MRotationY, heading) * Matrix3(MRotationX, dive);
}

#if 1

struct SecondaryContext
{
    float deltaT;
    float noAccDeltaT;
    Entity* cameraVehicle;
    bool insideVehicle;
    World* world;
};

static void DoBackgroundSimulate(void* context)
{
    SecondaryContext* sContext = static_cast<SecondaryContext*>(context);
    sContext->world->BackgroundSimulate(sContext->deltaT, sContext->noAccDeltaT, sContext->cameraVehicle,
                                        sContext->insideVehicle);
}
#endif


static void DrawConnectionQuality(ConnectionQuality quality)
{
    PackedColor color;
    switch (quality)
    {
        default:
            Fail("Connection quality");
        case CQGood:
            // color = PackedColor(Color(0, 1, 0, 0.5));
            // break;
            return;
        case CQPoor:
            color = PackedColor(Color(1, 1, 0, 0.5));
            break;
        case CQBad:
            color = PackedColor(Color(1, 0, 0, 0.5));
            break;
    }
    float w = GEngine->Width2D(), h = GEngine->Height2D();
    MipInfo mip = GEngine->TextBank()->UseMipmap(nullptr, 0, 0);
    GEngine->Draw2D(mip, color, Rect2DPixel(0.97 * w, 0.96 * h, 0.03 * w, 0.04 * h));
}

static void ClipCamera(Vector3& cam, Object* focus, Vector3Val focusPos, float& maxDistSmooth)
{
    // find point on focus->cam line that is furthers and has no collision

    CollisionBuffer col;
    // besides of direct line check also some area around
    const float sDist = 0.5;
    static const float coefs[][4] = {
        // up distance, aside distance, along distance, distance coef used
        {0, 0, 1, 1}, {sDist, 0, 1.2, 1.5}, {-sDist, 0, 1.2, 1.5}, {0, sDist, 1.2, 1.5}, {0, -sDist, 1.2, 1.5},
    };
    const int nCoefs = sizeof(coefs) / sizeof(*coefs);
    Vector3 dir = cam - focusPos;

    Vector3 up = VUp;
    Vector3 aside = dir.CrossProduct(up);
    aside.Normalize();
    up = dir.CrossProduct(aside);
    up.Normalize();
    /*
    LODShapeWithShadow *sphere = GScene->Preloaded(SphereModel);
    GScene->DrawDiagModel(focusPos+up*0.1,sphere);
    GScene->DrawDiagModel(focusPos+aside*0.1,sphere);
    */

    float dirSize2 = dir.SquareSize();
    float invDirSize = InvSqrt(dirSize2);
    float dirSize = dirSize2 * invDirSize;

    float minMaxDist = dirSize;
    Vector3 dirNorm = dir * invDirSize;

    int nCol = 0;
    for (int c = 0; c < nCoefs; c++)
    {
        float distTest = dirSize * coefs[c][2];
        Vector3 camPosAdj = cam + up * coefs[c][0] + aside * coefs[c][1] + dirNorm * coefs[c][2];
        GLandscape->ObjectCollision(col, focus, nullptr, focusPos, camPosAdj, 0.2, ObjIntersectView);
        if (col.Size() <= 0)
        {
            continue;
        }
        bool someCol = false;
        // find nearest t
        float minT = 1;
        for (int i = 0; i < col.Size(); i++)
        {
            const CollisionInfo& info = col[i];
            // ignore obstacle that are not opaque enough
            /*
            LOG_DEBUG(World,
                "%s: View density %.3f",
                (const char *)info.object->GetDebugName(),info.object->ViewDensity()
            );
            */
            if (info.object->ViewDensity() > -3.0f)
            {
                continue;
            }
            if (info.under < minT)
            {
                minT = info.under;
            }
            someCol = true;
        }

        if (someCol)
        {
            nCol++;
        }

        float maxDist = distTest * minT;
        maxDist -= 0.2f;
        saturateMax(maxDist, 0.5f);
        saturateMax(maxDist, 0.333f * dirSize);
        saturateMin(minMaxDist, maxDist * coefs[c][3]);
    }

    if (nCol > 3)
    {
        saturateMin(maxDistSmooth, minMaxDist);
    }
    cam = focusPos + dirNorm * floatMin(dirSize, maxDistSmooth);
}

