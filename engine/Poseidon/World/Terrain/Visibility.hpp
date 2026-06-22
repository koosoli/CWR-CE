#pragma once

// sensor dirty unit is 2 m (max dirty is 20 m)

#include <Poseidon/AI/VehicleAI.hpp>

namespace Poseidon
{
enum {MaxSensorDirty=15};

template <class VehicleType>
class SensorInfo
{
	friend class SensorRow;
	friend class SensorList;
	
	OLink<VehicleType> _vehicle;
	
public:
	LSError Serialize(ParamArchive &ar);
	bool IsDefaultValue(ParamArchive &ar) const {return false;}
	void LoadDefaultValues(ParamArchive &ar) {}

};

class SensorCol: public SensorInfo<EntityAI>
{
};

// Element of visibility matrix

struct SensorUpdate
{
	unsigned char vis8; // check result
	Foundation::TimeSec lastVisible; // last time when visibility was > 0
	Foundation::Time time; // last check time
	Vector3 rowPos,colPos; // sensor and target position

	public:
	void Init();
	bool IsDefaultValue(ParamArchive &ar) const;
	void LoadDefaultValues(ParamArchive &ar);

	LSError Serialize(ParamArchive &ar);
	RString DiagText() const;

	__forceinline float GetVisibility() const {return vis8*(1.0f/255);}
	__forceinline void SetVisibility0(){vis8=0;}
	__forceinline void SetVisibility1(){vis8=255;}

	void SetVisibility(float vis);
	void SetLastVisible(Foundation::Time vt){lastVisible=Foundation::TimeSec(vt);}
	Foundation::Time GetLastVisibilityTime() const;
};

// Row of visibility matrix

class SensorRow: public SensorInfo<Person>
{
	friend class SensorList;
	typedef SensorInfo<Person> base;

	AutoArray<SensorUpdate> _info;

	public:
	SensorRow();
	void Init( Person *vehicle, int size );
	void Free();	

	LSError Serialize(ParamArchive &ar);
};

// Visibility matrix
// rows are observers, columns are targets

class SensorList
{
	AutoArray<SensorCol> _cols; // columns - targets
	AutoArray<SensorRow> _rows; // rows - observers
	int _lastUpdateRow;
	int _lastUpdateCol;

	SensorRowID FindRowID( Person *veh );
	SensorColID FindColID( EntityAI *veh );

	void DeleteCol( SensorColID i ); // 
	void DeleteRow( SensorRowID i ); // 

	public:
	SensorList();
	~SensorList();

	SensorRowID AddRow( Person *veh ); // new ID
	SensorColID AddCol( EntityAI *veh ); // new ID
	void DeleteRow( Person *veh ); // 
	void DeleteCol( EntityAI *veh ); // 
	void Compact();

	// debugging helpers
	Foundation::Time RowUpdatedTime( SensorRowID id ) const;
	Foundation::Time ColUpdatedTime( SensorColID id ) const;
	Foundation::Time RowPosUpdatedTime( SensorRowID id ) const;
	Foundation::Time ColPosUpdatedTime( SensorColID id ) const;
	float RowPosMoved( SensorRowID id ) const;
	float ColPosMoved( SensorColID id ) const;
	int CellDirtyR( SensorRowID r, SensorColID c ) const;
	int CellDirtyC( SensorRowID r, SensorColID c ) const;

	void CheckPos();

	// all Updates return count of visibility checks
	int UpdateCell( SensorRowID r, SensorColID c ); // update single

	int UpdateRow( SensorRowID id ); // update single
	int UpdateCol( SensorColID id ); // update single
	int UpdateAll();
	int SmartUpdateAll(); // update what is necessary, only some
	float GetVisibility( Person *from, EntityAI *to ) const;
	Foundation::Time GetVisibilityTime( Person *from, EntityAI *to ) const;

	RString DiagText( Person *from, EntityAI *to ) const;

	bool CheckStructure() const;
	static float Unimportance
	(
		EntityAI *from, EntityAI *to, float &defValue
	);

	LSError Serialize(ParamArchive &ar);
	static SensorList *CreateObject(ParamArchive &ar) {return new SensorList();}
};

}  // namespace Poseidon
