#pragma once
#include <Poseidon/Foundation/PoseidonPCH.hpp>
#include <Poseidon/Core/Config/EngineConfig.hpp>
#include <Poseidon/World/Entities/Infantry/SoldierOld.hpp>
#include <Poseidon/World/Entities/Vehicles/Transport.hpp>
#include <Poseidon/World/Entities/Weapons/Shots.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/World.hpp>
#include <Poseidon/World/Terrain/Landscape.hpp>
#include <Random/randomGen.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>
#include <Poseidon/World/Entities/Vehicles/Tracks.hpp>
#include <Poseidon/World/Entities/Weapons/ProxyWeapon.hpp>
#include <Poseidon/Graphics/Rendering/Draw/SpecLods.hpp>
#include <Poseidon/World/Entities/Vehicles/House.hpp>
#include <Poseidon/World/Detection/Detector.hpp>
#include <Poseidon/Graphics/Textures/TexturePreload.hpp>
#include <Poseidon/Dev/Diag/DiagModes.hpp>
#include <Poseidon/Foundation/Strings/Bstring.hpp>
#include <Poseidon/World/Entities/Infantry/MoveActions.hpp>
#include <Poseidon/Network/Network.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include <Poseidon/Game/OperMap.hpp>
#include <Poseidon/Game/UiActions.hpp>
#include <Poseidon/Game/Commands/GameStateExt.hpp>
#include <Poseidon/Game/Scripting/Scripts.hpp>
#include <Poseidon/World/Entities/Vehicles/SeaGull.hpp>
#include <Poseidon/Foundation/Algorithms/Qsort.hpp>
#include <Poseidon/Foundation/Enums/EnumNames.hpp>
#include <Poseidon/Game/Chat.hpp>
#include <Poseidon/UI/Locale/StringtableExt.hpp>

#if _ENABLE_CHEATS
#define ARROWS 1
#else
#define ARROWS 0
#endif

#define DNAME() ((const char*)GetDebugName())
#define NAME(i) ((const char*)GetMoveName(i))
#define NAME_T(t, i) ((const char*)t->GetMoveName(i))

#include <Poseidon/Foundation/Containers/BankArray.hpp>

namespace Poseidon
{
struct ActionContextDefault : public ActionContextBase
{
    int param;
    int param2;
    RString param3;

    ActionContextDefault();
    ~ActionContextDefault() override;

    USE_FAST_ALLOCATOR;
};

struct ActionContextUIAction : public ActionContextBase
{
    UIAction action;

    USE_FAST_ALLOCATOR;
    ActionContextUIAction(const UIAction& uiAction);
};

struct ActionContextGetIn : public ActionContextBase
{
    OLink<Transport> vehicle;
    UIActionType position;

    USE_FAST_ALLOCATOR;
    ActionContextGetIn(Transport* veh, UIActionType pos);
};

template <>
struct BankTraits<AnimationRT>
{
    typedef const AnimationRTName& NameType;
    static int CompareNames(const AnimationRTName& n1, const AnimationRTName& n2)
    {
        int d = strcmpi(n1.name, n2.name);
        if (d) return d;
        d = (char*)n1.skeleton.GetRef() - (char*)n2.skeleton.GetRef();
        return d;
    }
    typedef RefArray<AnimationRT> ContainerType;
};

extern BankArray<AnimationRT> AnimationRTBank;
extern BankArray<WeightInfo> WeigthBank;

inline const float WalkSpeed = 1.5f;
inline const float WalkBackSpeed = 1.5f;
inline const float RunSpeed = 4.9f;
inline const float FastRunSpeed = 7.2f;
inline const float TiredRunSpeed = 4.9f;
inline const float InvFastRunSpeed = 1 / FastRunSpeed;
inline const float TurnSpeed = 1.0f;
inline const float TurnBackSpeed = 0.7f;

#define GRAD_0_SPEED 1.0f
#define GRAD_1_SPEED 1.0f
#define GRAD_2_SPEED 1.1f
#define GRAD_3_SPEED 1.2f
#define GRAD_4_SPEED 1.5f
#define GRAD_5_SPEED 2.0f
#define GRAD_6_SPEED 3.0f

inline const float GradPenalty[7] = {GRAD_0_SPEED, GRAD_1_SPEED, GRAD_2_SPEED, GRAD_3_SPEED,
                                     GRAD_4_SPEED, GRAD_5_SPEED, GRAD_6_SPEED};
inline const float InvGradPenalty[7] = {1 / GRAD_0_SPEED, 1 / GRAD_1_SPEED, 1 / GRAD_2_SPEED, 1 / GRAD_3_SPEED,
                                        1 / GRAD_4_SPEED, 1 / GRAD_5_SPEED, 1 / GRAD_6_SPEED};

inline float GradientPenalty(int grad)
{
    if (grad >= 7) return 1e30f;
    return GradPenalty[grad];
}
inline float GradientMaxSpeed(int grad)
{
    if (grad >= 7) return 0.1f;
    return InvGradPenalty[grad];
}

inline const float MaxStandSlope = 0.9f;
inline const float MinStandSlope = -0.9f;
inline const float MaxSprintSlope = 0.2f;
inline const float MinSprintSlope = -0.5f;
inline const float MinRunSlope = -0.7f;
inline const float MaxRunSlope = 0.4f;

inline bool CanSprint(float slope) { return slope >= MinSprintSlope && slope <= MaxSprintSlope; }
inline bool CanRun(float slope) { return slope >= MinRunSlope && slope <= MaxRunSlope; }
inline bool CanStand(float slope) { return slope >= MinStandSlope && slope <= MaxStandSlope; }

static inline int GetUpDegree(CombatMode cm, bool handGun)
{
    if (cm == CMSafe || cm == CMCareless)
    {
        return ManPosStand;
    }
    if (handGun)
    {
        return ManPosHandGunStand;
    }
    return ManPosCombat;
}

static inline void LimitLead(Vector3& spd, float maxLead)
{
    float size2 = spd.SquareSize();
    if (size2 > Square(maxLead))
    {
        spd.Normalize();
        spd *= maxLead;
    }
}

#define BLEND_ANIM(name)                                        \
    BlendAnimSelectionsStorage name##Storage;                   \
    BlendAnimSelections name;                                   \
    AUTO_STORAGE_ALIGNED(BlendAnimInfo, name##Memory, 32, int); \
    name##Storage.Init(name##Memory, sizeof(name##Memory));     \
    name.SetStorage(name##Storage);

}  // namespace Poseidon
