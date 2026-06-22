#pragma once


#ifndef _MSC_VER
  #include <Poseidon/AI/VehicleAI.hpp>
#endif

#define Directions								20
#define direction_delta						directions20

namespace Poseidon
{
} // namespace Poseidon


namespace Poseidon { extern int directions8[8][2]; }
namespace Poseidon { extern int directions20[20][3]; }



struct OperInfo
{
	float _cost;
	int _x,_z;
	
	OLink<Object> house;
	int from;
	int to;
};


DEFINE_ENUM_BEG(OperItemType)
	OITNormal,
	OITAvoidBush,
	OITAvoidTree,
	OITAvoid,
	OITWater,
	OITSpaceRoad,
	OITRoad,
	OITSpaceBush,
	OITSpaceTree,
	OITSpace,
	OITRoadForced,
	NOperItemType
DEFINE_ENUM_END(OperItemType)

struct OperItem
{
	float _cost;
	float _heur;

	OperItem* _parent;
	

	int _searchID;
	
	int _x;
	int _z;
	
	BYTE _dir;
	bool _open;

	SizedEnum<OperItemType, BYTE> _type;
	SizedEnum<OperItemType, BYTE> _typeSoldier;
};


#define MASK_AVOID_OBJECTS		1
#define MASK_AVOID_VEHICLES		2
#define MASK_PREFER_ROADS			4
#define MASK_USE_BUFFER				16


struct OperDoor
{
	OLink<Object> house;
	int exit;
	int x;
	int z;
};

namespace Poseidon { class OperCache; }
class OperField : public RefCount,public CLRefLink
{
friend class Poseidon::OperCache;
public:
	OperItem _items[OperItemRange][OperItemRange];
	AutoArray<OperDoor> _doors;
	int _x; //!< square grid coordinate - 0..LandRange
	int _z; //!< square grid coordinate - 0..LandRange
	float _heurCost;
	float _baseCost;

	Poseidon::Foundation::Time _lastUsed;

public:
	OperField(int x, int z, int mask) {Init(x, z, mask);}
	void Init(int x, int z, int mask) {_x = x; _z = z; CreateField(mask);}

	bool IsField(int x, int z) {return x == _x && z == _z;}
	float BaseCost() {return _baseCost;}
	float HeurCost() {return _heurCost;}

protected:
	void CreateField(int mask);
	enum RastMode {RMSet,RMMax,RMMin};
	void Rasterize
	(
		Object *obj, const Vector3 *minMax,
		RastMode mode, OperItemType type, float xResize, float zResize, bool soldier
	);

	// do not use fast allocator - OperField is too big
};

#include <Poseidon/Foundation/Memory/MemFreeReq.hpp>

class IOperCache
{
public:
	virtual OperField* GetOperField(int x, int z, int mask) = 0;
	virtual void RemoveField(OperField*fld) = 0;
	virtual void RemoveField(int x, int z) = 0;

	virtual ~IOperCache() {}

	virtual int NFields() const = 0;
};

IOperCache *CreateOperCache(Landscape *land);

#define FIELD_ON_DEMAND		1

typedef bool FindNearestEmptyCallback(Vector3Par pos, void *context);

namespace Poseidon { class AIGroup; class AIUnit; class Path; }
class OperMap
{
friend class Poseidon::AIGroup;
friend class Poseidon::AIUnit;
friend class Poseidon::Path;
friend class DebugWindowOperMap;
friend class ASOCostFunction;
friend class ASOIterator;
protected:
	StaticArray< Ref<OperField> > _fields;
	StaticArray<OperDoor> _doors;
	bool _alternateGoal;

public:
	StaticArray<OperInfo> _path;

	OperMap();
	~OperMap();

	void ClearMap();
	void CreateMap(EntityAI *veh, int xMin, int zMin, int xMax, int zMax);

	bool FindPath(AIUnit *unit, int dir, int xs, int zs, int xe, int ze, bool locks);
	bool IsSimplePath(AIUnit *unit, int xs, int zs, int xe, int ze);
	int ResultPath(AIUnit* unit);
	void InitStaticStorage();
	bool IsAlternateGoal() const {return _alternateGoal;}

protected:
	OperItem* Item(int x, int z);
	float GetFieldCost(int x, int z, bool locks, EntityAI *veh, bool soldier);
	float GetCost(int xf, int zf, int dir, bool locks, EntityAI *veh, bool soldier);
	bool FindNearestEmpty
	(
		int& x, int& z, float xf, float zf,
		int xbase, int zbase, int xsize, int zsize, bool locks,
		EntityAI *veh, bool soldier,
		FindNearestEmptyCallback *isFree, void *context
	);
	void LogMap(int xMin, int xMax, int zMin, int zMax, 
		int xs, int xe, int zs, int ze, bool path);
	bool IsIntersection(int xs, int zs, int xe, int ze,
				float &cost, float &costPerItem, EntityAI *veh, bool soldier);
	OperField *CreateField(int x, int z, int mask, EntityAI *veh);

	OperDoor *FindDoor(int x, int z);
	OperDoor *FindDoor(int xs, int zs, Object *house);
	bool FindDoor(OperInfo &info, int xs, int zs, int xe, int ze);
	Object *FindDoor(int xs, int zs, int xe, int ze);
};
