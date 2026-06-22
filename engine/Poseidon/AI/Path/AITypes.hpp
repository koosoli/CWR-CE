#pragma once

#include <Poseidon/World/Scene/Object.hpp>
#include <Poseidon/Foundation/Types/RemoveLinks.hpp>

#include <Poseidon/Foundation/Containers/BoolArray.hpp>

class OperField;

class IOperCache;

namespace Poseidon
{
class AIMap;
class AI;
class AILocker;
class AIUnit;
class AISubgroup;
class AIGroup;
class AICenter;

namespace Dev::DebugCommands { class Command; }
class Mission;
class Work;

class LockField;

class AITargetInfo;
class AICheckPointInfo;

#define MAX_UNITS_PER_GROUP		12

#define OperItemRange			16
#define OperItemGrid			(LandGrid / OperItemRange)
#define InvOperItemGrid		(OperItemRange * InvLandGrid)

#define BigFieldSize	8 // locking and pathfinding big field size

union GeographyInfo
{
	DWORD packed;
	struct
	{
		// note: some bug in MSVC 5.0
		// bool is not properly used in bitfield (in unions)
		unsigned waterDepth:2; // 0- no water, 2,3 - deep and very deep water
		unsigned full:1;
		unsigned forestInner:1;
		unsigned forestOuter:1;
		unsigned road:1;	// fast road
		unsigned track:1;
		unsigned slow:1; // mud, rocks etc.
		unsigned howManyObjects:2; // count of hard obstacles (0..3)
		unsigned howManyHardObjects:2; // count of hard obstacles (0..3)
		unsigned gradient:3;
	} u;
};


typedef VehicleWithAI TargetType;

class LockField
{
protected:
	friend class LockCache;
	friend class AILocker;

	BYTE _locks[OperItemRange][OperItemRange];
	LockField* _next;
	LockField* _prev;
	WORD _x;
	WORD _z;
	WORD _lock;

public:
	LockField(int x, int z);
	~LockField();

	bool IsLocked(int x, int z, bool soldier);
	void Lock(int x, int z, bool soldier, bool lock);
	
	USE_FAST_ALLOCATOR;
};

class ILockCache
{
public:
	virtual ~ILockCache() {}

	virtual int SearchID() = 0;

	virtual bool IsLocked(int x, int z, bool soldier) = 0;
	virtual LockField* GetLockField(int x, int z) = 0; 
	virtual void ReleaseLockField(int x, int z) = 0;
	virtual bool IsEmpty() const = 0;

	virtual LockField* FindLockField(int x, int z) = 0; // use for diagnostics
};

ILockCache *CreateLockCache(Landscape *land);

// Message types for InGameUI
enum MessageType
{
	MessageSent,
	MessageReceived,
	MessageMission
};

}  // namespace Poseidon
