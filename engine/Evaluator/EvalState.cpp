#include <Evaluator/EvalState.hpp>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <ctype.h>
#include <map>
#include <utility>
#include <vector>
#include <Poseidon/Foundation/Strings/RString.hpp>

#define NOTHING GameValue()

static EvalState* getEval(const GameState* state)
{
    return const_cast<EvalState*>(static_cast<const EvalState*>(state));
}

// A string yields its raw text; GetText() would wrap it in quotes.
static std::string displayText(GameValuePar arg)
{
    if (arg.GetType() == GameString)
    {
        RString text = arg;
        return std::string((const char*)text);
    }
    RString text = arg.GetText();
    return std::string((const char*)text);
}

static GameValue StubHint(const GameState* state, GameValuePar arg)
{
    RString text = arg;
    getEval(state)->appendOutput(std::string("[HINT] ") + (const char*)text + "\n");
    return NOTHING;
}

static GameValue StubHintC(const GameState* state, GameValuePar arg)
{
    RString text = arg;
    getEval(state)->appendOutput(std::string("[HINT] ") + (const char*)text + "\n");
    return NOTHING;
}

static GameValue StubLogInfo(const GameState* state, GameValuePar arg)
{
    getEval(state)->appendOutput(std::string("[LOG] ") + displayText(arg) + "\n");
    return NOTHING;
}

static GameValue StubTextLog(const GameState* state, GameValuePar arg)
{
    getEval(state)->appendOutput(std::string("[LOG] ") + displayText(arg) + "\n");
    return NOTHING;
}

static GameValue StubDebugLog(const GameState* state, GameValuePar arg)
{
    getEval(state)->appendOutput(std::string("[DEBUG] ") + displayText(arg) + "\n");
    return NOTHING;
}

static GameValue StubGlobalChat(const GameState* state, GameValuePar, GameValuePar arg2)
{
    RString text = arg2;
    getEval(state)->appendOutput(std::string("[CHAT] ") + (const char*)text + "\n");
    return NOTHING;
}

static GameValue StubSideChat(const GameState* state, GameValuePar, GameValuePar arg2)
{
    RString text = arg2;
    getEval(state)->appendOutput(std::string("[CHAT] ") + (const char*)text + "\n");
    return NOTHING;
}

static GameValue StubGroupChat(const GameState* state, GameValuePar, GameValuePar arg2)
{
    RString text = arg2;
    getEval(state)->appendOutput(std::string("[CHAT] ") + (const char*)text + "\n");
    return NOTHING;
}

static GameValue StubVehicleChat(const GameState* state, GameValuePar, GameValuePar arg2)
{
    RString text = arg2;
    getEval(state)->appendOutput(std::string("[CHAT] ") + (const char*)text + "\n");
    return NOTHING;
}

static GameValue EvalFormat(const GameState*, GameValuePar arg)
{
    const GameArrayType& array = arg;
    if (array.Size() < 1 || array[0].GetType() != GameString)
        return GameValue("");

    RString format = array[0];
    int nParams = array.Size() - 1;
    char result[2048];
    const char* src = format;
    char* dst = result;

    while (char c = *src)
    {
        if (c == '%')
        {
            src++;
            int index = 0;
            while (isdigit((unsigned char)*src))
            {
                index = index * 10 + (*src - '0');
                src++;
            }
            if (index < 1 || index > nParams)
                continue;
            const GameValue& value = array[index];
            RString text = (value.GetType() == GameString) ? value.GetData()->GetString() : value.GetText();
            strcpy(dst, text);
            dst += text.GetLength();
        }
        else
        {
            *dst++ = c;
            src++;
        }
    }
    *dst = 0;
    return GameValue(result);
}

static GameValue NularTime(const GameState*)
{
    return GameValue(0.0f);
}
static GameValue NularIsServer(const GameState*)
{
    return GameValue(true);
}
static GameValue NularObjNull(const GameState*)
{
    return makeObjectValue(0);
}
static GameValue NularGrpNull(const GameState*)
{
    return makeGroupValue(0);
}
static GameValue NularPlayer(const GameState* state)
{
    auto* eval = getEval(state);
    if (eval->playerId == 0)
    {
        eval->playerId = eval->objects.createObject("SoldierWB", 0, 0, 0);
        eval->objects.getObject(eval->playerId)->name = "player";
    }
    return makeObjectValue(eval->playerId);
}
static GameValue StubDiagLog(const GameState* state, GameValuePar arg)
{
    getEval(state)->appendOutput(std::string("[LOG] ") + displayText(arg) + "\n");
    return NOTHING;
}

static GameValue EvalToString(const GameState*, GameValuePar arg)
{
    return GameValue(arg.GetText());
}

static GameValue EvalToArray(const GameState*, GameValuePar arg)
{
    RString str = arg;
    GameArrayType arr;
    for (int i = 0; i < str.GetLength(); i++)
        arr.Add(GameValue((float)(unsigned char)str[i]));
    return GameValue(arr);
}

static GameValue EvalToStringFromArray(const GameState*, GameValuePar arg)
{
    const GameArrayType& arr = arg;
    std::string result;
    for (int i = 0; i < arr.Size(); i++)
        result += (char)(int)(float)arr[i];
    return GameValue(result.c_str());
}

void EvalState::RegisterEvalCommands()
{
    NewFunction(GameFunction(GameNothing, "hint", StubHint, GameString));
    NewFunction(GameFunction(GameNothing, "hintC", StubHintC, GameString));
    NewFunction(GameFunction(GameNothing, "hintCadet", StubHint, GameString));
    NewFunction(GameFunction(GameNothing, "logInfo", StubLogInfo, GameVoid));
    NewFunction(GameFunction(GameNothing, "textLog", StubTextLog, GameVoid));
    NewFunction(GameFunction(GameNothing, "debugLog", StubDebugLog, GameVoid));
    NewFunction(GameFunction(GameNothing, "diag_log", StubDiagLog, GameVoid));

    NewFunction(GameFunction(GameString, "format", EvalFormat, GameArray));

    NewFunction(GameFunction(GameString, "str", EvalToString, GameVoid));
    NewFunction(GameFunction(GameArray, "toArray", EvalToArray, GameString));
    NewFunction(GameFunction(GameString, "toString", EvalToStringFromArray, GameArray));

    NewOperator(GameOperator(GameNothing, "globalChat", function, StubGlobalChat, GameVoid, GameString));
    NewOperator(GameOperator(GameNothing, "sideChat", function, StubSideChat, GameVoid, GameString));
    NewOperator(GameOperator(GameNothing, "groupChat", function, StubGroupChat, GameVoid, GameString));
    NewOperator(GameOperator(GameNothing, "vehicleChat", function, StubVehicleChat, GameVoid, GameString));

    NewNularOp(GameNular(GameScalar, "time", NularTime));
    NewNularOp(GameNular(GameBool, "isServer", NularIsServer));
    NewNularOp(GameNular(MockGameObject, "objNull", NularObjNull));
    NewNularOp(GameNular(MockGameGroup, "grpNull", NularGrpNull));
    NewNularOp(GameNular(MockGameObject, "player", NularPlayer));

    RegisterObjectCommands();
    RegisterCreationCommands();
    RegisterPropertyCommands();
    RegisterVariableCommands();
    RegisterScriptCommands();
}

// --- Object manipulation commands ---

static GameValue CmdAlive(const GameState* state, GameValuePar arg)
{
    int id = getObjectId(arg);
    auto* obj = getEval(state)->objects.getObject(id);
    return GameValue(obj ? obj->alive : false);
}

static GameValue CmdTypeOf(const GameState* state, GameValuePar arg)
{
    int id = getObjectId(arg);
    auto* obj = getEval(state)->objects.getObject(id);
    return obj ? GameValue(obj->type.c_str()) : GameValue("");
}

static GameValue CmdName(const GameState* state, GameValuePar arg)
{
    int id = getObjectId(arg);
    auto* obj = getEval(state)->objects.getObject(id);
    return obj ? GameValue(obj->name.c_str()) : GameValue("");
}

static GameValue CmdGetPos(const GameState* state, GameValuePar arg)
{
    int id = getObjectId(arg);
    auto* obj = getEval(state)->objects.getObject(id);
    GameArrayType arr;
    if (obj)
    {
        arr.Add(GameValue(obj->pos[0]));
        arr.Add(GameValue(obj->pos[1]));
        arr.Add(GameValue(obj->pos[2]));
    }
    else
    {
        arr.Add(GameValue(0.0f));
        arr.Add(GameValue(0.0f));
        arr.Add(GameValue(0.0f));
    }
    return GameValue(arr);
}

static GameValue CmdGetDir(const GameState* state, GameValuePar arg)
{
    int id = getObjectId(arg);
    auto* obj = getEval(state)->objects.getObject(id);
    return GameValue(obj ? obj->dir : 0.0f);
}

static GameValue CmdGetDammage(const GameState* state, GameValuePar arg)
{
    int id = getObjectId(arg);
    auto* obj = getEval(state)->objects.getObject(id);
    return GameValue(obj ? obj->damage : 0.0f);
}

static GameValue CmdIsNull(const GameState* state, GameValuePar arg)
{
    if (!arg.GetData())
        return GameValue(true);
    if (arg.GetData()->GetType() == MockGameObject)
        return GameValue(getObjectId(arg) == 0);
    if (arg.GetData()->GetType() == MockGameGroup)
        return GameValue(getGroupId(arg) == 0);
    return GameValue(arg.GetData()->GetNil());
}

static GameValue CmdSetPos(const GameState* state, GameValuePar lhs, GameValuePar rhs)
{
    int id = getObjectId(lhs);
    auto* obj = getEval(state)->objects.getObject(id);
    if (obj)
    {
        const GameArrayType& arr = rhs;
        if (arr.Size() >= 3)
        {
            obj->pos[0] = (float)arr[0];
            obj->pos[1] = (float)arr[1];
            obj->pos[2] = (float)arr[2];
        }
    }
    return NOTHING;
}

static GameValue CmdSetDir(const GameState* state, GameValuePar lhs, GameValuePar rhs)
{
    int id = getObjectId(lhs);
    auto* obj = getEval(state)->objects.getObject(id);
    if (obj)
        obj->dir = (float)rhs;
    return NOTHING;
}

static GameValue CmdSetDammage(const GameState* state, GameValuePar lhs, GameValuePar rhs)
{
    int id = getObjectId(lhs);
    auto* obj = getEval(state)->objects.getObject(id);
    if (obj)
    {
        float dammage = (float)rhs;
        if (!obj->allowDammage && dammage > obj->damage)
            return NOTHING;

        obj->damage = dammage;
        obj->alive = obj->damage < 1.0f;
    }
    return NOTHING;
}

static GameValue CmdAllowDammage(const GameState* state, GameValuePar lhs, GameValuePar rhs)
{
    int id = getObjectId(lhs);
    auto* obj = getEval(state)->objects.getObject(id);
    if (obj)
        obj->allowDammage = (bool)rhs;
    return NOTHING;
}

void EvalState::RegisterObjectCommands()
{
    NewFunction(GameFunction(GameBool, "alive", CmdAlive, MockGameObject));
    NewFunction(GameFunction(GameString, "typeOf", CmdTypeOf, MockGameObject));
    NewFunction(GameFunction(GameString, "name", CmdName, MockGameObject));
    NewFunction(GameFunction(GameArray, "getPos", CmdGetPos, MockGameObject));
    NewFunction(GameFunction(GameScalar, "getDir", CmdGetDir, MockGameObject));
    NewFunction(GameFunction(GameScalar, "getDammage", CmdGetDammage, MockGameObject));
    NewFunction(GameFunction(GameBool, "isNull", CmdIsNull, GameVoid));

    NewOperator(GameOperator(GameNothing, "setPos", function, CmdSetPos, MockGameObject, GameArray));
    NewOperator(GameOperator(GameNothing, "setDir", function, CmdSetDir, MockGameObject, GameScalar));
    NewOperator(GameOperator(GameNothing, "setDammage", function, CmdSetDammage, MockGameObject, GameScalar));
    NewOperator(GameOperator(GameNothing, "setDamage", function, CmdSetDammage, MockGameObject, GameScalar));
    NewOperator(GameOperator(GameNothing, "allowDammage", function, CmdAllowDammage, MockGameObject, GameBool));
    // Grammatically-correct one-M alias resolves to the same handler (cf. setDamage/setDammage).
    NewOperator(GameOperator(GameNothing, "allowDamage", function, CmdAllowDammage, MockGameObject, GameBool));
}

// --- Creation commands ---

static GameValue NularWest(const GameState*)
{
    return makeSideValue(SideWest);
}
static GameValue NularEast(const GameState*)
{
    return makeSideValue(SideEast);
}
static GameValue NularResistance(const GameState*)
{
    return makeSideValue(SideResistance);
}
static GameValue NularCivilian(const GameState*)
{
    return makeSideValue(SideCivilian);
}
static GameValue NularSideLogic(const GameState*)
{
    return makeSideValue(SideLogic);
}

static GameValue CmdCreateVehicle(const GameState* state, GameValuePar arg)
{
    const GameArrayType& arr = arg;
    if (arr.Size() < 2)
        return makeObjectValue(0);
    RString type = arr[0];
    const GameArrayType& pos = arr[1];
    float x = pos.Size() > 0 ? (float)pos[0] : 0;
    float y = pos.Size() > 1 ? (float)pos[1] : 0;
    float z = pos.Size() > 2 ? (float)pos[2] : 0;
    return makeObjectValue(getEval(state)->objects.createObject((const char*)type, x, y, z));
}

static GameValue CmdCreateUnit(const GameState* state, GameValuePar arg)
{
    const GameArrayType& arr = arg;
    if (arr.Size() < 3)
        return makeObjectValue(0);
    RString type = arr[0];
    const GameArrayType& pos = arr[1];
    float x = pos.Size() > 0 ? (float)pos[0] : 0;
    float y = pos.Size() > 1 ? (float)pos[1] : 0;
    float z = pos.Size() > 2 ? (float)pos[2] : 0;
    auto* eval = getEval(state);
    int id = eval->objects.createObject((const char*)type, x, y, z);
    int gid = getGroupId(arr[2]);
    auto* grp = eval->groups.getGroup(gid);
    if (grp)
    {
        grp->unitIds.push_back(id);
        if (grp->leaderId < 0)
            grp->leaderId = id;
        eval->objects.getObject(id)->groupId = gid;
    }
    return makeObjectValue(id);
}

static GameValue CmdDeleteVehicle(const GameState* state, GameValuePar arg)
{
    getEval(state)->objects.deleteObject(getObjectId(arg));
    return NOTHING;
}

static GameValue CmdCreateGroup(const GameState* state, GameValuePar arg)
{
    return makeGroupValue(getEval(state)->groups.createGroup(getMockSide(arg)));
}

static GameValue CmdCreateCenter(const GameState*, GameValuePar arg)
{
    return GameValue(arg.GetData()->Clone());
}

static GameValue CmdObjGroup(const GameState* state, GameValuePar arg)
{
    auto* obj = getEval(state)->objects.getObject(getObjectId(arg));
    return obj ? makeGroupValue(obj->groupId) : makeGroupValue(0);
}

static GameValue CmdUnits(const GameState* state, GameValuePar arg)
{
    auto* grp = getEval(state)->groups.getGroup(getGroupId(arg));
    GameArrayType arr;
    if (grp)
        for (int uid : grp->unitIds)
            arr.Add(makeObjectValue(uid));
    return GameValue(arr);
}

static GameValue CmdLeader(const GameState* state, GameValuePar arg)
{
    auto* grp = getEval(state)->groups.getGroup(getGroupId(arg));
    return grp ? makeObjectValue(grp->leaderId) : makeObjectValue(0);
}

static GameValue CmdJoin(const GameState* state, GameValuePar lhs, GameValuePar rhs)
{
    int objId = getObjectId(lhs);
    int newGid = getGroupId(rhs);
    auto* eval = getEval(state);
    auto* obj = eval->objects.getObject(objId);
    if (!obj)
        return NOTHING;
    if (obj->groupId > 0)
    {
        auto* oldGrp = eval->groups.getGroup(obj->groupId);
        if (oldGrp)
        {
            auto& ids = oldGrp->unitIds;
            ids.erase(std::remove(ids.begin(), ids.end(), objId), ids.end());
            if (oldGrp->leaderId == objId)
                oldGrp->leaderId = ids.empty() ? -1 : ids[0];
        }
    }
    auto* newGrp = eval->groups.getGroup(newGid);
    if (newGrp)
    {
        newGrp->unitIds.push_back(objId);
        if (newGrp->leaderId < 0)
            newGrp->leaderId = objId;
    }
    obj->groupId = newGid;
    return NOTHING;
}

void EvalState::RegisterCreationCommands()
{
    NewNularOp(GameNular(MockGameSide, "west", NularWest));
    NewNularOp(GameNular(MockGameSide, "east", NularEast));
    NewNularOp(GameNular(MockGameSide, "resistance", NularResistance));
    NewNularOp(GameNular(MockGameSide, "civilian", NularCivilian));
    NewNularOp(GameNular(MockGameSide, "sideLogic", NularSideLogic));

    NewFunction(GameFunction(MockGameObject, "createVehicle", CmdCreateVehicle, GameArray));
    NewFunction(GameFunction(MockGameObject, "createUnit", CmdCreateUnit, GameArray));
    NewFunction(GameFunction(GameNothing, "deleteVehicle", CmdDeleteVehicle, MockGameObject));
    NewFunction(GameFunction(MockGameGroup, "createGroup", CmdCreateGroup, MockGameSide));
    NewFunction(GameFunction(MockGameSide, "createCenter", CmdCreateCenter, MockGameSide));
    NewFunction(GameFunction(MockGameGroup, "group", CmdObjGroup, MockGameObject));
    NewFunction(GameFunction(GameArray, "units", CmdUnits, MockGameGroup));
    NewFunction(GameFunction(MockGameObject, "leader", CmdLeader, MockGameGroup));

    NewOperator(GameOperator(GameNothing, "join", function, CmdJoin, MockGameObject, MockGameGroup));
}
// --- Property & utility commands ---

static GameValue CmdDistance(const GameState* state, GameValuePar lhs, GameValuePar rhs)
{
    auto* eval = getEval(state);
    auto* o1 = eval->objects.getObject(getObjectId(lhs));
    auto* o2 = eval->objects.getObject(getObjectId(rhs));
    if (!o1 || !o2)
        return GameValue(0.0f);
    float dx = o1->pos[0] - o2->pos[0];
    float dy = o1->pos[1] - o2->pos[1];
    float dz = o1->pos[2] - o2->pos[2];
    return GameValue(std::sqrt(dx * dx + dy * dy + dz * dz));
}

static GameValue CmdVehicle(const GameState*, GameValuePar arg)
{
    return GameValue(arg.GetData()->Clone());
}

static GameValue CmdSide(const GameState* state, GameValuePar arg)
{
    auto* eval = getEval(state);
    auto* obj = eval->objects.getObject(getObjectId(arg));
    if (!obj)
        return makeSideValue(SideUnknown);
    auto* grp = eval->groups.getGroup(obj->groupId);
    return grp ? makeSideValue(grp->side) : makeSideValue(SideUnknown);
}

static GameValue CmdRating(const GameState*, GameValuePar)
{
    return GameValue(0.0f);
}
static GameValue CmdScore(const GameState*, GameValuePar)
{
    return GameValue(0.0f);
}
static GameValue CmdSpeed(const GameState*, GameValuePar)
{
    return GameValue(0.0f);
}
static GameValue CmdFuel(const GameState*, GameValuePar)
{
    return GameValue(1.0f);
}

static GameValue CmdDamage(const GameState* state, GameValuePar arg)
{
    auto* obj = getEval(state)->objects.getObject(getObjectId(arg));
    return GameValue(obj ? obj->damage : 0.0f);
}

static GameValue CmdCountType(const GameState* state, GameValuePar arg)
{
    const GameArrayType& arr = arg;
    if (arr.Size() < 2)
        return GameValue(0.0f);
    RString type = arr[0];
    const GameArrayType& objects = arr[1];
    int count = 0;
    auto* eval = getEval(state);
    for (int i = 0; i < objects.Size(); i++)
    {
        auto* obj = eval->objects.getObject(getObjectId(objects[i]));
        if (obj && obj->type == (const char*)type)
            count++;
    }
    return GameValue((float)count);
}

static GameValue CmdNearestObject(const GameState* state, GameValuePar arg)
{
    const GameArrayType& arr = arg;
    if (arr.Size() < 2)
        return makeObjectValue(0);
    const GameArrayType& pos = arr[0];
    RString type = arr[1];
    float px = pos.Size() > 0 ? (float)pos[0] : 0;
    float py = pos.Size() > 1 ? (float)pos[1] : 0;
    float pz = pos.Size() > 2 ? (float)pos[2] : 0;
    auto* eval = getEval(state);
    int bestId = 0;
    float bestDist = 1e30f;
    for (auto& [id, obj] : eval->objects.allObjects())
    {
        if (obj.type != (const char*)type)
            continue;
        float dx = obj.pos[0] - px, dy = obj.pos[1] - py, dz = obj.pos[2] - pz;
        float d = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (d < bestDist)
        {
            bestDist = d;
            bestId = id;
        }
    }
    return makeObjectValue(bestId);
}

static GameValue CmdRemoveAllEH(const GameState*, GameValuePar, GameValuePar)
{
    return NOTHING;
}

void EvalState::RegisterPropertyCommands()
{
    NewOperator(GameOperator(GameScalar, "distance", function, CmdDistance, MockGameObject, MockGameObject));

    NewFunction(GameFunction(MockGameObject, "vehicle", CmdVehicle, MockGameObject));
    NewFunction(GameFunction(MockGameSide, "side", CmdSide, MockGameObject));
    NewFunction(GameFunction(GameScalar, "rating", CmdRating, MockGameObject));
    NewFunction(GameFunction(GameScalar, "score", CmdScore, MockGameObject));
    NewFunction(GameFunction(GameScalar, "speed", CmdSpeed, MockGameObject));
    NewFunction(GameFunction(GameScalar, "fuel", CmdFuel, MockGameObject));
    NewFunction(GameFunction(GameScalar, "damage", CmdDamage, MockGameObject));
    NewFunction(GameFunction(GameScalar, "countType", CmdCountType, GameArray));
    NewFunction(GameFunction(MockGameObject, "nearestObject", CmdNearestObject, GameArray));

    NewOperator(
        GameOperator(GameNothing, "removeAllEventHandlers", function, CmdRemoveAllEH, MockGameObject, GameString));
}
// --- Variable & action commands ---

static GameValue CmdSetVariable(const GameState* state, GameValuePar lhs, GameValuePar rhs)
{
    auto* obj = getEval(state)->objects.getObject(getObjectId(lhs));
    if (!obj)
        return NOTHING;
    const GameArrayType& arr = rhs;
    if (arr.Size() < 2)
        return NOTHING;
    RString name = arr[0];
    obj->vars[(const char*)name] = arr[1];
    return NOTHING;
}

static GameValue CmdGetVariable(const GameState* state, GameValuePar lhs, GameValuePar rhs)
{
    auto* obj = getEval(state)->objects.getObject(getObjectId(lhs));
    if (!obj)
        return NOTHING;
    RString name = rhs;
    auto it = obj->vars.find((const char*)name);
    return it != obj->vars.end() ? it->second : NOTHING;
}

static GameValue CmdAddAction(const GameState* state, GameValuePar lhs, GameValuePar rhs)
{
    auto* obj = getEval(state)->objects.getObject(getObjectId(lhs));
    if (!obj)
        return GameValue(0.0f);
    return GameValue((float)obj->nextActionId++);
}

static GameValue CmdRemoveAction(const GameState*, GameValuePar, GameValuePar)
{
    return NOTHING;
}

static GameValue CmdAnimate(const GameState*, GameValuePar, GameValuePar)
{
    return NOTHING;
}
static GameValue CmdAnimationPhase(const GameState*, GameValuePar, GameValuePar)
{
    return GameValue(0.0f);
}
static GameValue CmdSwitchMove(const GameState*, GameValuePar, GameValuePar)
{
    return NOTHING;
}
static GameValue CmdPlayMove(const GameState*, GameValuePar, GameValuePar)
{
    return NOTHING;
}
static GameValue CmdEnableSimulation(const GameState*, GameValuePar, GameValuePar)
{
    return NOTHING;
}
static GameValue CmdSetIdentity(const GameState*, GameValuePar, GameValuePar)
{
    return NOTHING;
}

void EvalState::RegisterVariableCommands()
{
    NewOperator(GameOperator(GameNothing, "setVariable", function, CmdSetVariable, MockGameObject, GameArray));
    NewOperator(GameOperator(GameVoid, "getVariable", function, CmdGetVariable, MockGameObject, GameString));
    NewOperator(GameOperator(GameScalar, "addAction", function, CmdAddAction, MockGameObject, GameArray));
    NewOperator(GameOperator(GameNothing, "removeAction", function, CmdRemoveAction, MockGameObject, GameArray));

    NewOperator(GameOperator(GameNothing, "animate", function, CmdAnimate, MockGameObject, GameArray));
    NewOperator(GameOperator(GameScalar, "animationPhase", function, CmdAnimationPhase, MockGameObject, GameString));
    NewOperator(GameOperator(GameNothing, "switchMove", function, CmdSwitchMove, MockGameObject, GameString));
    NewOperator(GameOperator(GameNothing, "playMove", function, CmdPlayMove, MockGameObject, GameString));
    NewOperator(GameOperator(GameNothing, "enableSimulation", function, CmdEnableSimulation, MockGameObject, GameBool));
    NewOperator(GameOperator(GameNothing, "setIdentity", function, CmdSetIdentity, MockGameObject, GameString));
}

// --- Script commands (exec, call, exit, exitWith) ---
#include <Evaluator/SqsRunner.hpp>
#include <fstream>
#include <sstream>

static std::string readFileContent(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open())
        return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// In-game exec is non-blocking; the evaluator runs the SQS synchronously.
static GameValue CmdExec(const GameState* state, GameValuePar lhs, GameValuePar rhs)
{
    RString filename = rhs;
    std::string content = readFileContent((const char*)filename);
    if (content.empty())
        return NOTHING;

    auto* eval = getEval(state);
    SqsRunner runner((const char*)filename);
    runner.parse(content);
    runner.run(*eval, lhs);
    return NOTHING;
}

static GameValue CmdCall(const GameState* state, GameValuePar lhs, GameValuePar rhs)
{
    // lhs becomes _this for the evaluated block.
    RString code = rhs;
    auto* eval = getEval(state);
    GameVarSpace local(eval->GetContext());
    eval->BeginContext(&local);
    eval->VarSetLocal("_this", lhs, true);
    GameValue result = eval->EvaluateMultiple(code);
    eval->EndContext();
    return result;
}

// No-op: the evaluator has no script scope to terminate.
static GameValue CmdExit(const GameState*)
{
    return NOTHING;
}

static GameValue CmdExitWith(const GameState*, GameValuePar arg)
{
    return GameValue(arg.GetData() ? arg.GetData()->Clone() : nullptr);
}

void EvalState::RegisterScriptCommands()
{
    NewOperator(GameOperator(GameNothing, "exec", function, CmdExec, GameVoid, GameString));
    NewOperator(GameOperator(GameVoid, "call", function, CmdCall, GameVoid, GameString));
    NewNularOp(GameNular(GameNothing, "exit", CmdExit));
    NewFunction(GameFunction(GameVoid, "exitWith", CmdExitWith, GameVoid));
}
