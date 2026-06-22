#pragma once
#include <Evaluator/express.hpp>
#include <map>
#include <string>
#include <vector>
#include <cmath>

// Mock type constants (same values as Poseidon's GameStateExt.hpp)
const GameType MockGameObject(0x100);
const GameType MockGameSide(0x1000);
const GameType MockGameGroup(0x2000);

// Side enum matching TargetSide
enum MockSide
{
    SideWest,
    SideEast,
    SideResistance,
    SideCivilian,
    SideUnknown,
    SideLogic
};

struct MockObject
{
    int id;
    std::string type;
    std::string name;
    float pos[3];
    float dir;
    float damage;
    bool allowDammage;
    bool alive;
    int groupId;
    int nextActionId;
    std::map<std::string, GameValue> vars;

    MockObject() : id(0), pos{0, 0, 0}, dir(0), damage(0), allowDammage(true), alive(true), groupId(-1), nextActionId(0)
    {}
};

struct MockGroup
{
    int id;
    MockSide side;
    std::vector<int> unitIds;
    int leaderId;

    MockGroup() : id(0), side(SideUnknown), leaderId(-1) {}
};

class ObjectRegistry
{
    std::map<int, MockObject> _objects;
    int _nextId = 1;

  public:
    int createObject(const std::string& type, float x, float y, float z)
    {
        int id = _nextId++;
        MockObject obj;
        obj.id = id;
        obj.type = type;
        obj.name = type;
        obj.pos[0] = x;
        obj.pos[1] = y;
        obj.pos[2] = z;
        _objects[id] = obj;
        return id;
    }
    MockObject* getObject(int id)
    {
        auto it = _objects.find(id);
        return it != _objects.end() ? &it->second : nullptr;
    }
    void deleteObject(int id) { _objects.erase(id); }
    const std::map<int, MockObject>& allObjects() const { return _objects; }
};

class GroupRegistry
{
    std::map<int, MockGroup> _groups;
    int _nextId = 1;

  public:
    int createGroup(MockSide side)
    {
        int id = _nextId++;
        MockGroup grp;
        grp.id = id;
        grp.side = side;
        _groups[id] = grp;
        return id;
    }
    MockGroup* getGroup(int id)
    {
        auto it = _groups.find(id);
        return it != _groups.end() ? &it->second : nullptr;
    }
    const std::map<int, MockGroup>& allGroups() const { return _groups; }
};

// Mock GameData for objects (uses integer ID instead of real Object*)
class MockGameDataObject : public GameData
{
    int _id;

  public:
    MockGameDataObject() : _id(0) {}
    MockGameDataObject(int id) : _id(id) {}
    GameType GetType() const override { return MockGameObject; }
    int GetObjectId() const { return _id; }
    RString GetText() const override
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "<object %d>", _id);
        return RString(buf);
    }
    bool IsEqualTo(const GameData* data) const override
    {
        return _id == static_cast<const MockGameDataObject*>(data)->_id;
    }
    const char* GetTypeName() const override { return "object"; }
    GameData* Clone() const override { return new MockGameDataObject(*this); }
    bool GetNil() const override { return false; }
};

class MockGameDataGroup : public GameData
{
    int _id;

  public:
    MockGameDataGroup() : _id(0) {}
    MockGameDataGroup(int id) : _id(id) {}
    GameType GetType() const override { return MockGameGroup; }
    int GetGroupId() const { return _id; }
    RString GetText() const override
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "<group %d>", _id);
        return RString(buf);
    }
    bool IsEqualTo(const GameData* data) const override
    {
        return _id == static_cast<const MockGameDataGroup*>(data)->_id;
    }
    const char* GetTypeName() const override { return "group"; }
    GameData* Clone() const override { return new MockGameDataGroup(*this); }
    bool GetNil() const override { return false; }
};

class MockGameDataSide : public GameData
{
    MockSide _side;

  public:
    MockGameDataSide() : _side(SideUnknown) {}
    MockGameDataSide(MockSide s) : _side(s) {}
    GameType GetType() const override { return MockGameSide; }
    MockSide GetSide() const { return _side; }
    RString GetText() const override
    {
        switch (_side)
        {
            case SideWest:
                return "WEST";
            case SideEast:
                return "EAST";
            case SideResistance:
                return "GUER";
            case SideCivilian:
                return "CIV";
            case SideLogic:
                return "LOGIC";
            default:
                return "UNKNOWN";
        }
    }
    bool IsEqualTo(const GameData* data) const override
    {
        return _side == static_cast<const MockGameDataSide*>(data)->_side;
    }
    const char* GetTypeName() const override { return "side"; }
    GameData* Clone() const override { return new MockGameDataSide(*this); }
};

inline int getObjectId(GameValuePar val)
{
    if (val.GetData() && val.GetData()->GetType() == MockGameObject)
        return static_cast<const MockGameDataObject*>(val.GetData())->GetObjectId();
    return 0;
}

inline int getGroupId(GameValuePar val)
{
    if (val.GetData() && val.GetData()->GetType() == MockGameGroup)
        return static_cast<const MockGameDataGroup*>(val.GetData())->GetGroupId();
    return 0;
}

inline MockSide getMockSide(GameValuePar val)
{
    if (val.GetData() && val.GetData()->GetType() == MockGameSide)
        return static_cast<const MockGameDataSide*>(val.GetData())->GetSide();
    return SideUnknown;
}

inline GameValue makeObjectValue(int id)
{
    return GameValue(new MockGameDataObject(id));
}

inline GameValue makeGroupValue(int id)
{
    return GameValue(new MockGameDataGroup(id));
}

inline GameValue makeSideValue(MockSide s)
{
    return GameValue(new MockGameDataSide(s));
}
