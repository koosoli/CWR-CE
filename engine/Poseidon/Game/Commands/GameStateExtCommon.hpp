#pragma once

#include <Poseidon/AI/VehicleAI.hpp>
namespace Poseidon
{
} // namespace Poseidon

#include <Poseidon/Game/Commands/GameStateExt.hpp>

#define NOTHING GameValue()
#define OBJECT_NULL GameValueExt((Object*)nullptr)
#define GROUP_NULL GameValueExt((AIGroup*)nullptr)

#define DEFINE_COMMAND(Xxx)                                                                   \
    GameValue VehDo##Xxx(const GameState* state, GameValuePar oper1, GameValuePar oper2)      \
    {                                                                                         \
        return Veh##Xxx(state, oper1, oper2, true);                                           \
    }                                                                                         \
    GameValue VehCommand##Xxx(const GameState* state, GameValuePar oper1, GameValuePar oper2) \
    {                                                                                         \
        return Veh##Xxx(state, oper1, oper2, false);                                          \
    }

#define DEFINE_COMMAND_S(Xxx)                                             \
    GameValue VehDo##Xxx(const GameState* state, GameValuePar oper1)      \
    {                                                                     \
        return Veh##Xxx(state, oper1, true);                              \
    }                                                                     \
    GameValue VehCommand##Xxx(const GameState* state, GameValuePar oper1) \
    {                                                                     \
        return Veh##Xxx(state, oper1, false);                             \
    }

extern const GameType GameObjectOrGroup;
extern const GameType GameObjectOrArray;

// Shared helpers
using Poseidon::Target;
Object* GetObject(GameValuePar oper);
AIGroup* GetGroup(GameValuePar oper);
Target* FindTargetReveal(AIGroup* grp, EntityAI* veh);
bool GetVector(Vector3& ret, GameValuePar oper);
bool GetRelPos(const GameState* state, Vector3& ret, GameValuePar oper);
bool GetPos(const GameState* state, Vector3& ret, GameValuePar oper);
bool GetPos(Vector3& ret, GameValuePar oper);

// Weapon/magazine pool commands (defined in OptionsUI.cpp, namespace Poseidon)
namespace Poseidon
{
GameValue PoolAddWeapon(const GameState* state, GameValuePar oper1);
GameValue PoolAddMagazine(const GameState* state, GameValuePar oper1);
GameValue PoolGetWeapons(const GameState* state, GameValuePar oper1);
GameValue PoolSetWeapons(const GameState* state, GameValuePar oper1);
GameValue PoolClearWeapons(const GameState* state);
GameValue PoolClearMagazines(const GameState* state);
GameValue PoolQueryWeapons(const GameState* state, GameValuePar oper1);
GameValue PoolQueryMagazines(const GameState* state, GameValuePar oper1);
} // namespace Poseidon

// Factory functions
GameData* CreateGameDataObject();
GameData* CreateGameDataGroup();
GameData* CreateGameDataSide();
GameData* CreateGameDataFile();
GameValue CreateGameObject(Object* obj);

// Config file entries (shared between compilation units)
struct ConfigFileEntry
{
    ParamClass* cls;
    int refCount;

    ConfigFileEntry()
    {
        cls = nullptr;
        refCount = 0;
    }
    ConfigFileEntry(ParamClass* c)
    {
        cls = c;
        refCount = 0;
    }
};
class ConfigFileEntries : public AutoArray<ConfigFileEntry>
{
    typedef AutoArray<ConfigFileEntry> base;

  public:
    int Add(const ConfigFileEntry& item);
};
extern ConfigFileEntries GFileEntries;

// Cross-file helpers
GameValue ObjNull(const GameState* state);
AIUnit* GetUnit(const GameState* state, GameValuePar oper1);
bool OpenScript(QIFStreamB& in, RString name);

// Inline validation helpers (used across split files)
inline bool CheckSize(const GameState* state, GameArrayType array, int size)
{
    if (array.Size() == size)
    {
        return true;
    }
    state->SetError(EvalDim, array.Size(), size);
    return false;
}

inline bool CheckType(const GameState* state, GameValuePar oper, GameType type)
{
    if (oper.GetType() == type)
    {
        return true;
    }
    state->TypeError(type, oper.GetType());
    return false;
}
